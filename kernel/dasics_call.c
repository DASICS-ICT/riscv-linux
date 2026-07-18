// SPDX-License-Identifier: GPL-2.0-only

#include <linux/dasics.h>
#include <linux/errno.h>
#include <linux/export.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/percpu.h>
#include <linux/preempt.h>
#include <linux/string.h>

#include <asm/csr.h>

static DEFINE_PER_CPU(struct dasics_call_frame *, dasics_active_call_frame);

static bool dasics_call_transition_valid(enum dasics_call_frame_state from,
					 enum dasics_call_frame_state next)
{
	switch (from) {
	case DASICS_CALL_FRAME_EMPTY:
		return next == DASICS_CALL_FRAME_PREPARED ||
		       next == DASICS_CALL_FRAME_CLEANED;
	case DASICS_CALL_FRAME_PREPARED:
		return next == DASICS_CALL_FRAME_ENTERED ||
		       next == DASICS_CALL_FRAME_CLEANED;
	case DASICS_CALL_FRAME_ENTERED:
		return next == DASICS_CALL_FRAME_RETURNED ||
		       next == DASICS_CALL_FRAME_FAULTED ||
		       next == DASICS_CALL_FRAME_CLEANED;
	case DASICS_CALL_FRAME_RETURNED:
	case DASICS_CALL_FRAME_FAULTED:
		return next == DASICS_CALL_FRAME_CLEANED;
	case DASICS_CALL_FRAME_CLEANED:
		return next == DASICS_CALL_FRAME_CLEANED;
	default:
		return false;
	}
}

static int dasics_call_transition(struct dasics_call_frame *frame,
				  enum dasics_call_frame_state next,
				  bool warn)
{
	bool valid = frame && frame->magic == DASICS_CALL_FRAME_MAGIC &&
		     dasics_call_transition_valid(frame->state, next);

	if (!valid) {
		if (frame && frame->magic == DASICS_CALL_FRAME_MAGIC)
			frame->state_error = -EPROTO;
		if (warn)
			WARN_ON_ONCE(!valid);
		return -EPROTO;
	}

	frame->state = next;
	return 0;
}

int dasics_compartment_init_module(struct dasics_compartment *compartment,
				   void *target)
{
#ifdef CONFIG_MODULES
	struct module *module;
	unsigned long address = (unsigned long)target;
	unsigned int nr_ranges = 0;

	if (!compartment || !target)
		return -EINVAL;

	preempt_disable();
	module = __module_text_address(address);
	if (!module || !module_is_live(module)) {
		preempt_enable();
		return -ENOENT;
	}

	memset(compartment, 0, sizeof(*compartment));
	compartment->module = module;
	if (module->core_layout.text_size) {
		compartment->loader_code_ranges[nr_ranges].base =
			(unsigned long)module->core_layout.base;
		compartment->loader_code_ranges[nr_ranges].size =
			module->core_layout.text_size;
		nr_ranges++;
	}
	if (module->init_layout.text_size) {
		compartment->loader_code_ranges[nr_ranges].base =
			(unsigned long)module->init_layout.base;
		compartment->loader_code_ranges[nr_ranges].size =
			module->init_layout.text_size;
		nr_ranges++;
	}
	preempt_enable();

	if (!nr_ranges)
		return -ENOENT;
	compartment->code_ranges = compartment->loader_code_ranges;
	compartment->nr_code_ranges = nr_ranges;
	return 0;
#else
	return -EOPNOTSUPP;
#endif
}
EXPORT_SYMBOL_GPL(dasics_compartment_init_module);

static int dasics_call_register_regions(struct dasics_call_frame *frame,
					const struct dasics_call_policy *policy,
					unsigned long stack_hi)
{
	struct dasics_region stack;
	unsigned int i;
	int ret;

	ret = dasics_bound_table_init(&frame->data_bounds, policy->callee,
				      DASICS_DATA_BOUND_SLOTS);
	if (ret)
		return ret;

	if (!policy->stack_size)
		return -EINVAL;
	if (policy->stack_size > stack_hi)
		return -EOVERFLOW;
	stack.base = stack_hi - policy->stack_size;
	stack.size = policy->stack_size;
	stack.perms = DASICS_REGION_READ | DASICS_REGION_WRITE;
	ret = dasics_bound_register(&frame->data_bounds, &stack,
				    DASICS_BOUND_LIFETIME_CALL,
				    DASICS_BOUND_PINNED,
				    &frame->stack_handle);
	if (ret)
		return ret;

	for (i = 0; i < frame->nr_normalized; i++) {
		const struct dasics_region *region = &frame->normalized[i];
		dasics_bound_handle_t handle;
		unsigned long region_hi = region->base + region->size;

		/* The pinned RW stack grant already covers contained objects. */
		if (region->base >= stack.base && region_hi <= stack_hi)
			continue;
		ret = dasics_bound_register(&frame->data_bounds, region,
					    DASICS_BOUND_LIFETIME_CALL, 0,
					    &handle);
		if (ret)
			return ret;
	}

	return 0;
}

static unsigned long dasics_entry_cfg(const struct dasics_bound_entry *entry)
{
	unsigned long cfg = DASICS_LIBCFG_V;

	if (entry->perms & DASICS_REGION_READ)
		cfg |= DASICS_LIBCFG_R;
	if (entry->perms & DASICS_REGION_WRITE)
		cfg |= DASICS_LIBCFG_W;
	return cfg;
}

static int dasics_call_reside_bound(struct dasics_call_frame *frame,
				    struct dasics_hw_state *child,
				    dasics_bound_handle_t handle,
				    unsigned int slot)
{
	const struct dasics_bound_entry *entry;
	int ret;

	ret = dasics_bound_get(&frame->data_bounds, handle, &entry);
	if (ret)
		return ret;
	ret = dasics_bound_commit_resident(&frame->data_bounds, handle, slot);
	if (ret)
		return ret;

	child->lib_lo[slot] = entry->lo;
	child->lib_hi[slot] = entry->hi;
	child->libcfg |= dasics_entry_cfg(entry) <<
			 (slot * DASICS_LIBCFG_BITS);
	frame->nr_resident++;
	return 0;
}

static int dasics_call_build_child_state(struct dasics_call_frame *frame,
					 const struct dasics_call_policy *policy,
					 struct dasics_hw_state *child)
{
	unsigned int slot = 0;
	unsigned int i;
	int ret;

	memset(child, 0, sizeof(*child));
	child->dmaincall = frame->parent_hw.dmaincall;

	ret = dasics_call_reside_bound(frame, child, frame->stack_handle, slot++);
	if (ret)
		return ret;

	for (i = 0; i < ARRAY_SIZE(frame->data_bounds.entries) &&
		     slot < DASICS_DATA_BOUND_SLOTS; i++) {
		const struct dasics_bound_entry *entry =
			&frame->data_bounds.entries[i];

		if (!entry->active || entry->handle == frame->stack_handle)
			continue;
		ret = dasics_call_reside_bound(frame, child, entry->handle, slot++);
		if (ret)
			return ret;
	}

	ret = dasics_jump_policy_validate(policy->callee->nr_code_ranges);
	if (ret)
		return ret;
	for (i = 0; i < policy->callee->nr_code_ranges; i++) {
		const struct dasics_code_range *range =
			&policy->callee->code_ranges[i];

		if (!range->size || range->size > ULONG_MAX - range->base)
			return range->size ? -EOVERFLOW : -EINVAL;
		child->jump_lo[i] = range->base;
		child->jump_hi[i] = range->base + range->size;
		child->jumpcfg |= DASICS_JUMPCFG_V <<
				  (i * DASICS_JUMPCFG_BITS);
	}
	frame->nr_jump_bounds = policy->callee->nr_code_ranges;
	return 0;
}

static int dasics_call_restore_parent(struct dasics_call_frame *frame)
{
	int ret = 0;

	if (frame->parent_saved) {
		if (frame->test_fail_restore)
			ret = -EIO;
		else
			ret = dasics_hw_restore(&frame->parent_hw);
		if (ret) {
			dasics_hw_clear_call_authority();
			frame->finish_error = ret;
		}
		frame->parent_saved = false;
	}

	if (frame->preempt_held) {
		if (this_cpu_read(dasics_active_call_frame) == frame)
			this_cpu_write(dasics_active_call_frame, NULL);
		frame->preempt_held = false;
		preempt_enable();
	}
	if (dasics_call_transition(frame, DASICS_CALL_FRAME_CLEANED, true))
		frame->state = DASICS_CALL_FRAME_CLEANED;
	return ret;
}

static int dasics_call_fail_closed(struct dasics_call_frame *frame, int error)
{
	if (frame && frame->magic == DASICS_CALL_FRAME_MAGIC) {
		frame->state_error = error;
		dasics_call_restore_parent(frame);
	}
	return error;
}

int dasics_call_prepare(struct dasics_call_frame *frame,
			const struct dasics_call_policy *policy)
{
	struct dasics_hw_state child;
	unsigned int fail_bound;
	unsigned int fail_restore;
	unsigned long stack_hi;
	int ret;

	if (!frame || !policy)
		return -EINVAL;
	if (policy->flags)
		return -EOPNOTSUPP;
	if (frame->magic && frame->magic != DASICS_CALL_FRAME_MAGIC)
		return -EINVAL;
	if (frame->magic == DASICS_CALL_FRAME_MAGIC &&
	    frame->state != DASICS_CALL_FRAME_CLEANED &&
	    frame->state != DASICS_CALL_FRAME_EMPTY)
		return dasics_call_fail_closed(frame, -EBUSY);

	fail_bound = frame->magic == DASICS_CALL_FRAME_MAGIC ?
		     frame->test_fail_bound : 0;
	fail_restore = frame->magic == DASICS_CALL_FRAME_MAGIC ?
		       frame->test_fail_restore : 0;
	memset(frame, 0, sizeof(*frame));
	frame->magic = DASICS_CALL_FRAME_MAGIC;
	frame->state = DASICS_CALL_FRAME_EMPTY;
	frame->test_fail_bound = fail_bound;
	frame->test_fail_restore = fail_restore;
	frame->policy = policy;
	stack_hi = (unsigned long)__builtin_frame_address(0);

	ret = dasics_policy_normalize(policy, frame->normalized,
				      ARRAY_SIZE(frame->normalized),
				      &frame->nr_normalized);
	if (ret)
		goto fail;
	if (frame->nr_normalized >= DASICS_POLICY_MAX_REGIONS) {
		ret = -E2BIG;
		goto fail;
	}
	ret = dasics_call_register_regions(frame, policy, stack_hi);
	if (ret)
		goto fail;

	preempt_disable();
	frame->preempt_held = true;
	if (this_cpu_read(dasics_active_call_frame)) {
		ret = -EBUSY;
		goto fail;
	}
	this_cpu_write(dasics_active_call_frame, frame);

	ret = dasics_hw_save(&frame->parent_hw);
	if (ret)
		goto fail;
	frame->parent_saved = true;

	ret = dasics_call_build_child_state(frame, policy, &child);
	if (ret)
		goto fail;
	ret = dasics_hw_install_call_authority(&child,
					       frame->test_fail_bound);
	if (ret)
		goto fail;

	frame->prepare_error = 0;
	return dasics_call_transition(frame, DASICS_CALL_FRAME_PREPARED, true);

fail:
	frame->prepare_error = ret;
	dasics_call_restore_parent(frame);
	return ret;
}
EXPORT_SYMBOL_GPL(dasics_call_prepare);

int dasics_call_recover(int error)
{
	struct dasics_call_frame *frame;
	int ret;

	frame = this_cpu_read(dasics_active_call_frame);
	if (!frame || frame->magic != DASICS_CALL_FRAME_MAGIC)
		return -ENOENT;
	if (!error)
		error = -EFAULT;
	if (error > 0)
		error = -error;

	ret = dasics_call_transition(frame, DASICS_CALL_FRAME_FAULTED, true);
	if (ret)
		return dasics_call_fail_closed(frame, ret);
	frame->fault_error = error;
	return 0;
}
EXPORT_SYMBOL_GPL(dasics_call_recover);

static int dasics_call_refill_data_bound(struct dasics_call_frame *frame,
					 unsigned long address,
					 unsigned int access,
					 unsigned int *slot)
{
	const struct dasics_bound_entry *entry;
	dasics_bound_handle_t handle;
	dasics_bound_handle_t victim;
	unsigned int selected;
	int ret;

	ret = dasics_bound_find(&frame->data_bounds, address, access, &handle);
	if (ret)
		return ret;
	ret = dasics_bound_get(&frame->data_bounds, handle, &entry);
	if (ret)
		return ret;
	if (entry->resident_slot >= 0)
		return -EACCES;
	ret = dasics_bound_select_slot(&frame->data_bounds, handle, &selected,
				       &victim);
	if (ret)
		return ret;
	ret = dasics_hw_replace_data_bound(selected, entry->lo, entry->hi,
					   dasics_entry_cfg(entry));
	if (ret)
		return ret;
	ret = dasics_bound_commit_resident(&frame->data_bounds, handle, selected);
	if (ret) {
		dasics_hw_clear_call_authority();
		return -EUCLEAN;
	}
	frame->data_refills++;
	if (slot)
		*slot = selected;
	return 0;
}

int dasics_call_handle_trap(unsigned long pc, unsigned long address,
			    unsigned long reason, unsigned long cause,
			    const struct dasics_compartment **compartment,
			    unsigned int *slot)
{
	struct dasics_call_frame *frame;
	unsigned int access;
	int ret;

	frame = this_cpu_read(dasics_active_call_frame);
	if (!frame || frame->magic != DASICS_CALL_FRAME_MAGIC)
		return -ENOENT;
	if (frame->state != DASICS_CALL_FRAME_ENTERED)
		return dasics_call_fail_closed(frame, -EPROTO);
	if (!frame->policy || !frame->policy->callee)
		return dasics_call_fail_closed(frame, -EPROTO);
	if (compartment)
		*compartment = frame->policy->callee;

	if (reason == DASICS_FAULT_LOAD || reason == DASICS_FAULT_STORE) {
		access = reason == DASICS_FAULT_LOAD ? DASICS_REGION_READ :
			 DASICS_REGION_WRITE;
		frame->data_misses++;
		ret = dasics_call_refill_data_bound(frame, address, access, slot);
		if (!ret)
			return DASICS_TRAP_RETRY;
		if (ret != -ENOENT && ret != -EACCES && ret != -ENOSPC)
			return dasics_call_fail_closed(frame, ret);
	}

	frame->fault.pc = pc;
	frame->fault.address = address;
	frame->fault.reason = reason;
	frame->fault.cause = cause;
	frame->fault.compartment = frame->policy->callee;
	frame->fault.valid = true;
	ret = dasics_call_recover(-EFAULT);
	if (ret)
		frame->fault.valid = false;
	return ret ?: DASICS_TRAP_TERMINAL;
}

void dasics_call_finish(struct dasics_call_frame *frame)
{
	if (!frame || !frame->magic)
		return;
	if (frame->magic != DASICS_CALL_FRAME_MAGIC)
		return;
	if (frame->state == DASICS_CALL_FRAME_CLEANED &&
	    !frame->parent_saved && !frame->preempt_held)
		return;

	dasics_call_restore_parent(frame);
}
EXPORT_SYMBOL_GPL(dasics_call_finish);

long dasics_call(struct dasics_call_frame *frame,
		 const struct dasics_call_policy *policy,
		 struct dasics_call_regs *regs)
{
	long ret;

	if (!frame || !policy || !regs)
		return -EINVAL;

	ret = dasics_call_prepare(frame, policy);
	if (ret)
		return ret;

	regs->target = (unsigned long)policy->target;
	ret = dasics_call_transition(frame, DASICS_CALL_FRAME_ENTERED, true);
	if (ret) {
		dasics_call_fail_closed(frame, ret);
		return ret;
	}
	ret = dasics_hw_call(regs);
	if (frame->state == DASICS_CALL_FRAME_ENTERED) {
		int state_ret;

		state_ret = dasics_call_transition(frame,
						   DASICS_CALL_FRAME_RETURNED,
						   true);
		if (!ret)
			ret = state_ret;
	} else if (frame->state == DASICS_CALL_FRAME_FAULTED) {
		ret = frame->fault_error ?: -EFAULT;
	} else if (!ret) {
		ret = -EPROTO;
		frame->state_error = ret;
	}
	dasics_call_finish(frame);
	if (!ret && frame->finish_error)
		ret = frame->finish_error;
	return ret;
}
EXPORT_SYMBOL_GPL(dasics_call);

#ifdef CONFIG_DASICS_DEBUG
int dasics_call_test_fail_bound(struct dasics_call_frame *frame,
				unsigned int bound)
{
	if (!frame)
		return -EINVAL;
	if (frame->magic && frame->magic != DASICS_CALL_FRAME_MAGIC)
		return -EINVAL;
	if (frame->magic == DASICS_CALL_FRAME_MAGIC &&
	    frame->state != DASICS_CALL_FRAME_CLEANED &&
	    frame->state != DASICS_CALL_FRAME_EMPTY)
		return -EBUSY;
	if (!frame->magic) {
		memset(frame, 0, sizeof(*frame));
		frame->magic = DASICS_CALL_FRAME_MAGIC;
	}
	frame->test_fail_bound = bound;
	return 0;
}
EXPORT_SYMBOL_GPL(dasics_call_test_fail_bound);

int dasics_call_test_fail_restore(struct dasics_call_frame *frame, bool fail)
{
	if (!frame)
		return -EINVAL;
	if (frame->magic && frame->magic != DASICS_CALL_FRAME_MAGIC)
		return -EINVAL;
	if (frame->magic == DASICS_CALL_FRAME_MAGIC &&
	    frame->state != DASICS_CALL_FRAME_CLEANED &&
	    frame->state != DASICS_CALL_FRAME_EMPTY)
		return -EBUSY;
	if (!frame->magic) {
		memset(frame, 0, sizeof(*frame));
		frame->magic = DASICS_CALL_FRAME_MAGIC;
	}
	frame->test_fail_restore = fail;
	return 0;
}
EXPORT_SYMBOL_GPL(dasics_call_test_fail_restore);

int dasics_call_test_transition(struct dasics_call_frame *frame,
				enum dasics_call_frame_state next)
{
	int ret;

	if (!frame || frame->magic != DASICS_CALL_FRAME_MAGIC)
		return -EINVAL;
	ret = dasics_call_transition(frame, next, false);
	if (ret)
		dasics_call_fail_closed(frame, ret);
	return ret;
}
EXPORT_SYMBOL_GPL(dasics_call_test_transition);

int dasics_call_test_handle_trap(unsigned long pc, unsigned long address,
				 unsigned long reason, unsigned long cause)
{
	return dasics_call_handle_trap(pc, address, reason, cause, NULL, NULL);
}
EXPORT_SYMBOL_GPL(dasics_call_test_handle_trap);
#endif
