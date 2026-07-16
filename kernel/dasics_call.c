// SPDX-License-Identifier: GPL-2.0-only

#include <linux/dasics.h>
#include <linux/errno.h>
#include <linux/export.h>
#include <linux/kernel.h>
#include <linux/percpu.h>
#include <linux/preempt.h>
#include <linux/string.h>

#include <asm/csr.h>

static DEFINE_PER_CPU(struct dasics_call_frame *, dasics_active_call_frame);

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
		} else {
			frame->parent_saved = false;
		}
	}

	if (frame->preempt_held) {
		if (this_cpu_read(dasics_active_call_frame) == frame)
			this_cpu_write(dasics_active_call_frame, NULL);
		frame->preempt_held = false;
		preempt_enable();
	}
	frame->state = DASICS_CALL_FRAME_FINISHED;
	return ret;
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
	    frame->state == DASICS_CALL_FRAME_PREPARED)
		return -EBUSY;

	fail_bound = frame->magic == DASICS_CALL_FRAME_MAGIC ?
		     frame->test_fail_bound : 0;
	fail_restore = frame->magic == DASICS_CALL_FRAME_MAGIC ?
		       frame->test_fail_restore : 0;
	memset(frame, 0, sizeof(*frame));
	frame->magic = DASICS_CALL_FRAME_MAGIC;
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
	frame->state = DASICS_CALL_FRAME_PREPARED;
	return 0;

fail:
	frame->prepare_error = ret;
	dasics_call_restore_parent(frame);
	return ret;
}
EXPORT_SYMBOL_GPL(dasics_call_prepare);

void dasics_call_finish(struct dasics_call_frame *frame)
{
	if (!frame || !frame->magic)
		return;
	if (frame->magic != DASICS_CALL_FRAME_MAGIC)
		return;
	if (frame->state == DASICS_CALL_FRAME_FINISHED &&
	    !frame->parent_saved && !frame->preempt_held)
		return;

	dasics_call_restore_parent(frame);
}
EXPORT_SYMBOL_GPL(dasics_call_finish);

#ifdef CONFIG_DASICS_DEBUG
int dasics_call_test_fail_bound(struct dasics_call_frame *frame,
				unsigned int bound)
{
	if (!frame)
		return -EINVAL;
	if (frame->magic && frame->magic != DASICS_CALL_FRAME_MAGIC)
		return -EINVAL;
	if (frame->magic == DASICS_CALL_FRAME_MAGIC &&
	    frame->state == DASICS_CALL_FRAME_PREPARED)
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
	    frame->state == DASICS_CALL_FRAME_PREPARED)
		return -EBUSY;
	if (!frame->magic) {
		memset(frame, 0, sizeof(*frame));
		frame->magic = DASICS_CALL_FRAME_MAGIC;
	}
	frame->test_fail_restore = fail;
	return 0;
}
EXPORT_SYMBOL_GPL(dasics_call_test_fail_restore);
#endif
