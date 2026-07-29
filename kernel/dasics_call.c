// SPDX-License-Identifier: GPL-2.0-only

#include <linux/cpu.h>
#include <linux/dasics.h>
#include <linux/errno.h>
#include <linux/export.h>
#include <linux/irqflags.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/percpu.h>
#include <linux/preempt.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/string.h>

#include <asm/csr.h>

static DEFINE_PER_CPU(struct dasics_call_frame *, dasics_active_call_frame);
/*
 * Assembly reads this trusted shadow before it has a safe stack on which to
 * call a C per-CPU helper.  The current milestone is single hart and one
 * active owner.  A sleepable maincall may temporarily release preemption,
 * but another task is rejected while this shadow remains owned.
 */
struct dasics_call_frame *dasics_maincall_active_frame;

struct dasics_maincall_service {
	unsigned long id;
	unsigned int flags;
	long (*invoke)(const struct dasics_maincall_request *request,
		       unsigned long *value);
};

enum dasics_maincall_service_flags {
	DASICS_MAINCALL_MAY_SLEEP = BIT(0),
};

#ifdef CONFIG_DASICS_DEBUG
static dasics_maincall_debug_handler_t dasics_maincall_debug_handler;
#endif

struct dasics_call_frame *dasics_call_current_frame(void)
{
	struct dasics_call_frame *frame;

	preempt_disable();
	frame = this_cpu_read(dasics_active_call_frame);
	if (frame && frame->owner != current)
		frame = NULL;
	preempt_enable();
	return frame;
}
EXPORT_SYMBOL_GPL(dasics_call_current_frame);

static bool dasics_compartment_contains_pc(
		const struct dasics_compartment *compartment, unsigned long pc)
{
	unsigned int i;

	if (!compartment || !compartment->code_ranges)
		return false;
	for (i = 0; i < compartment->nr_code_ranges; i++) {
		const struct dasics_code_range *range =
			&compartment->code_ranges[i];

		if (range->size && range->size <= ULONG_MAX - range->base &&
		    pc >= range->base && pc < range->base + range->size)
			return true;
	}
	return false;
}

static int dasics_compartment_add_layout(
		struct dasics_compartment *compartment,
		const struct module_layout *layout)
{
	struct dasics_code_range *code;
	struct dasics_region *data;
	unsigned long base;

	if (!layout->base || !layout->size)
		return 0;
	if (layout->text_size > layout->ro_size ||
	    layout->ro_size > layout->ro_after_init_size ||
	    layout->ro_after_init_size > layout->size) {
		pr_err("DASICS invalid module layout base=%px size=%u text=%u ro=%u ro_after_init=%u\n",
		       layout->base, layout->size, layout->text_size,
		       layout->ro_size, layout->ro_after_init_size);
		return -EINVAL;
	}
	base = (unsigned long)layout->base;
	if (layout->text_size) {
		if (compartment->nr_code_ranges >=
		    ARRAY_SIZE(compartment->loader_code_ranges))
			return -E2BIG;
		code = &compartment->loader_code_ranges[
			compartment->nr_code_ranges++];
		code->base = base;
		code->size = layout->text_size;
	}

	/*
	 * Loader-owned metadata (including struct module) shares the writable
	 * core allocation with module data. Never authorize that tail as a
	 * blanket module self-grant. Text/RO data are safe to read
	 * automatically; mutable module objects must appear explicitly in the
	 * per-entry policy.
	 */
	if (layout->ro_after_init_size) {
		if (compartment->nr_data_ranges >=
		    ARRAY_SIZE(compartment->loader_data_ranges))
			return -E2BIG;
		data = &compartment->loader_data_ranges[
			compartment->nr_data_ranges++];
		data->base = base;
		data->size = layout->ro_after_init_size;
		data->perms = DASICS_REGION_READ;
	}

	return 0;
}

static int dasics_compartment_init_loader(
		struct dasics_compartment *compartment, struct module *module,
		void *target, bool include_init)
{
	enum module_state expected = include_init ? MODULE_STATE_COMING :
						   MODULE_STATE_GOING;
	int ret;

	if (!compartment || !module || !target ||
	    READ_ONCE(module->state) != expected)
		return -EINVAL;

	memset(compartment, 0, sizeof(*compartment));
	compartment->module = module;
	ret = dasics_compartment_add_layout(compartment, &module->core_layout);
	if (ret)
		return ret;
	if (include_init) {
		ret = dasics_compartment_add_layout(compartment,
						    &module->init_layout);
		if (ret)
			return ret;
	}
	compartment->code_ranges = compartment->loader_code_ranges;
	compartment->data_ranges = compartment->loader_data_ranges;
	compartment->registered = true;

	return dasics_compartment_contains_pc(compartment,
					      (unsigned long)target) ? 0 : -EPERM;
}

static long dasics_maincall_abi_info(
		const struct dasics_maincall_request *request,
		unsigned long *value)
{
	unsigned int i;

	switch (request->args[0]) {
	case DASICS_MAINCALL_QUERY_RUNTIME:
		for (i = 1; i < ARRAY_SIZE(request->args); i++) {
			if (request->args[i])
				return -EINVAL;
		}
		*value = DASICS_MAINCALL_ABI_VERSION;
		return 0;
	case DASICS_MAINCALL_QUERY_REGISTERS:
		for (i = 1; i < ARRAY_SIZE(request->args); i++) {
			if (request->args[i] != i + 1)
				return -EINVAL;
		}
		*value = DASICS_MAINCALL_REGISTER_TEST_VALUE;
		return 0;
	default:
		return -EINVAL;
	}
}

#ifdef CONFIG_DASICS_DEBUG
static long dasics_maincall_debug_nested(
		const struct dasics_maincall_request *request,
		unsigned long *value)
{
	dasics_maincall_debug_handler_t handler;

	handler = READ_ONCE(dasics_maincall_debug_handler);
	if (!handler)
		return -ENOSYS;
	return handler(request, value);
}

static long dasics_maincall_debug_sleep(
		const struct dasics_maincall_request *request,
		unsigned long *value)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(request->args); i++) {
		if (request->args[i])
			return -EINVAL;
	}
	schedule_timeout_uninterruptible(1);
	*value = DASICS_MAINCALL_SLEEP_TEST_VALUE;
	return 0;
}

int dasics_maincall_debug_register(dasics_maincall_debug_handler_t handler)
{
	if (!handler)
		return -EINVAL;
	if (cmpxchg(&dasics_maincall_debug_handler, NULL, handler))
		return -EBUSY;
	return 0;
}
EXPORT_SYMBOL_GPL(dasics_maincall_debug_register);

void dasics_maincall_debug_unregister(
		dasics_maincall_debug_handler_t handler)
{
	if (handler)
		cmpxchg(&dasics_maincall_debug_handler, handler, NULL);
}
EXPORT_SYMBOL_GPL(dasics_maincall_debug_unregister);
#endif

static const struct dasics_maincall_service dasics_maincall_services[] = {
	{
		.id = DASICS_MAINCALL_SERVICE_ABI_INFO,
		.invoke = dasics_maincall_abi_info,
	},
#ifdef CONFIG_DASICS_DEBUG
	{
		.id = DASICS_MAINCALL_SERVICE_DEBUG_NESTED,
		.invoke = dasics_maincall_debug_nested,
	},
	{
		.id = DASICS_MAINCALL_SERVICE_DEBUG_SLEEP,
		.flags = DASICS_MAINCALL_MAY_SLEEP,
		.invoke = dasics_maincall_debug_sleep,
	},
#endif
};

static int dasics_maincall_suspend(struct dasics_call_frame *frame,
				   const struct dasics_maincall_request *request)
{
	int ret;

	if (!frame || !request || frame->owner != current ||
	    frame->maincall_suspended || !frame->preempt_held ||
	    frame->caller_preempt_count || frame->caller_irqs_disabled ||
	    !(request->irq_status & SR_IE) || !irqs_disabled())
		return -EWOULDBLOCK;

	ret = dasics_hw_save(&frame->suspended_hw);
	if (ret)
		return ret;
	frame->maincall_suspended = true;
	frame->preempt_held = false;
	preempt_enable_no_resched();
	local_irq_enable();
	preempt_check_resched();
	return 0;
}

static int dasics_maincall_resume(struct dasics_call_frame *frame)
{
	int ret = 0;

	preempt_disable();
	local_irq_disable();
	frame->preempt_held = true;
	if (!frame->maincall_suspended || frame->owner != current ||
	    this_cpu_read(dasics_active_call_frame) != frame ||
	    READ_ONCE(dasics_maincall_active_frame) != frame)
		ret = -EPROTO;
	else
		ret = dasics_hw_restore(&frame->suspended_hw);
	frame->maincall_suspended = false;
	if (ret)
		dasics_hw_clear_call_authority();
	return ret;
}

asmlinkage struct dasics_maincall_request *dasics_maincall_dispatch(
		struct dasics_maincall_request *request)
{
	struct dasics_call_frame *frame;
	unsigned int i;

	request->status = -EPERM;
	request->value = 0;
	frame = dasics_call_current_frame();
	if (!frame || frame->magic != DASICS_CALL_FRAME_MAGIC ||
	    frame->state != DASICS_CALL_FRAME_ENTERED || !frame->policy ||
	    frame->owner != current || !frame->policy->callee ||
	    request != &frame->maincall_request ||
	    !dasics_compartment_contains_pc(frame->policy->callee,
					    request->return_pc)) {
		/* Never return to an untrusted-supplied PC after source rejection. */
		request->return_pc = csr_read(CSR_DRETURNPC);
		if (frame && frame->magic == DASICS_CALL_FRAME_MAGIC &&
		    frame->state == DASICS_CALL_FRAME_ENTERED)
			dasics_call_recover(-EPERM);
		return request;
	}

	for (i = 0; i < ARRAY_SIZE(dasics_maincall_services); i++) {
		const struct dasics_maincall_service *service =
			&dasics_maincall_services[i];

		if (service->id != request->service_id)
			continue;
		if (service->flags & DASICS_MAINCALL_MAY_SLEEP) {
			long service_status;
			int ret;

			ret = dasics_maincall_suspend(frame, request);
			if (ret) {
				request->status = ret;
				return request;
			}
			service_status = service->invoke(request,
							 &request->value);
			ret = dasics_maincall_resume(frame);
			request->status = ret ?: service_status;
			if (ret)
				dasics_call_recover(ret);
			return request;
		}
		request->status = service->invoke(request, &request->value);
		return request;
	}
	request->status = -ENOSYS;
	return request;
}

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
	int ret;

	if (!compartment || !target)
		return -EINVAL;
	if (compartment->loader_managed && compartment->registered)
		return -EBUSY;

	preempt_disable();
	module = __module_text_address(address);
	if (!module || !try_module_get(module)) {
		preempt_enable();
		return -ENOENT;
	}
	if (READ_ONCE(module->state) != MODULE_STATE_LIVE ||
	    !within_module_core(address, module) ||
	    !module->core_layout.text_size) {
		module_put(module);
		preempt_enable();
		return -ENOENT;
	}

	memset(compartment, 0, sizeof(*compartment));
	compartment->module = module;
	ret = dasics_compartment_add_layout(compartment, &module->core_layout);
	if (ret) {
		module_put(module);
		preempt_enable();
		return ret;
	}
	compartment->code_ranges = compartment->loader_code_ranges;
	compartment->data_ranges = compartment->loader_data_ranges;
	compartment->loader_managed = true;
	compartment->registered = true;
	preempt_enable();

	return 0;
#else
	return -EOPNOTSUPP;
#endif
}
EXPORT_SYMBOL_GPL(dasics_compartment_init_module);

void dasics_compartment_destroy(struct dasics_compartment *compartment)
{
#ifdef CONFIG_MODULES
	struct module *module;

	if (!compartment || !compartment->loader_managed ||
	    !compartment->registered)
		return;

	preempt_disable();
	module = compartment->module;
	WRITE_ONCE(compartment->registered, false);
	compartment->module = NULL;
	compartment->code_ranges = NULL;
	compartment->nr_code_ranges = 0;
	compartment->data_ranges = NULL;
	compartment->nr_data_ranges = 0;
	memset(compartment->loader_code_ranges, 0,
	       sizeof(compartment->loader_code_ranges));
	memset(compartment->loader_data_ranges, 0,
	       sizeof(compartment->loader_data_ranges));
	preempt_enable();
	module_put(module);
#endif
}
EXPORT_SYMBOL_GPL(dasics_compartment_destroy);

static int dasics_call_get_module(struct dasics_call_frame *frame,
				  const struct dasics_call_policy *policy)
{
#ifdef CONFIG_MODULES
	struct dasics_compartment *callee = policy->callee;
	struct module *module;
	enum module_state state;

	if (!callee->loader_managed)
		return 0;
	if (!READ_ONCE(callee->registered))
		return -ENODEV;
	module = READ_ONCE(callee->module);
	if (!module || !try_module_get(module))
		return -ENODEV;

	state = READ_ONCE(module->state);
	if (state != MODULE_STATE_LIVE &&
	    (!(policy->flags & DASICS_CALL_ALLOW_COMING) ||
	     state != MODULE_STATE_COMING)) {
		module_put(module);
		return state == MODULE_STATE_COMING ? -EBUSY : -ENODEV;
	}
	if (!READ_ONCE(callee->registered) ||
	    READ_ONCE(callee->module) != module) {
		module_put(module);
		return -ENODEV;
	}
	frame->callee_module = module;
	frame->module_ref_held = true;
#endif
	return 0;
}

static int dasics_call_register_regions(struct dasics_call_frame *frame,
					const struct dasics_call_policy *policy,
					unsigned long stack_hi)
{
	const struct dasics_compartment *callee = policy->callee;
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

	if (callee->nr_data_ranges && !callee->data_ranges)
		return -EINVAL;
	for (i = 0; i < callee->nr_data_ranges; i++) {
		const struct dasics_region *region = &callee->data_ranges[i];
		dasics_bound_handle_t handle;

		ret = dasics_bound_register(&frame->data_bounds, region,
					    DASICS_BOUND_LIFETIME_CALL, 0,
					    &handle);
		if (ret)
			return ret;
	}

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
	child->dmaincall = (unsigned long)dasics_maincall_gate;

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
	struct dasics_call_frame *top;
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

	if (frame->active_pushed) {
		top = this_cpu_read(dasics_active_call_frame);
		if (top == frame) {
			this_cpu_write(dasics_active_call_frame, frame->parent);
			WRITE_ONCE(dasics_maincall_active_frame, frame->parent);
		} else {
			dasics_hw_clear_call_authority();
			WRITE_ONCE(dasics_maincall_active_frame, NULL);
			frame->finish_error = -EPROTO;
			ret = -EPROTO;
		}
		frame->active_pushed = false;
	}
	if (frame->module_ref_held) {
		frame->module_ref_held = false;
		module_put(frame->callee_module);
		frame->callee_module = NULL;
	}
	if (frame->preempt_held) {
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
	/*
	 * The assembly maincall bridge intentionally uses one trusted top
	 * pointer. Refuse calls instead of silently sharing it across harts.
	 */
	if (num_online_cpus() != 1)
		return -EOPNOTSUPP;
	if (policy->flags & ~DASICS_CALL_ALLOW_COMING)
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
	frame->owner = current;
	frame->caller_preempt_count = preempt_count();
	frame->caller_irqs_disabled = irqs_disabled();
	stack_hi = (unsigned long)__builtin_frame_address(0);

	ret = dasics_call_get_module(frame, policy);
	if (ret)
		goto fail;
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
	frame->parent = this_cpu_read(dasics_active_call_frame);
	if (frame->parent) {
		if (frame->parent->owner != current) {
			ret = -EBUSY;
			goto fail;
		}
		if (frame->parent == frame) {
			ret = -EBUSY;
			goto fail;
		}
		if (frame->parent->magic != DASICS_CALL_FRAME_MAGIC ||
		    frame->parent->state != DASICS_CALL_FRAME_ENTERED) {
			ret = -EPROTO;
			goto fail;
		}
		if (frame->parent->depth >= DASICS_CALL_MAX_DEPTH - 1) {
			ret = -EOVERFLOW;
			goto fail;
		}
		frame->depth = frame->parent->depth + 1;
	}

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

	this_cpu_write(dasics_active_call_frame, frame);
	WRITE_ONCE(dasics_maincall_active_frame, frame);
	frame->active_pushed = true;
	frame->prepare_error = 0;
	ret = dasics_call_transition(frame, DASICS_CALL_FRAME_PREPARED, true);
	if (ret)
		goto fail;
	return 0;

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
	if (!frame || frame->owner != current ||
	    frame->magic != DASICS_CALL_FRAME_MAGIC)
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
	if (!frame || frame->owner != current ||
	    frame->magic != DASICS_CALL_FRAME_MAGIC)
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
	ret = dasics_call_recover(reason == DASICS_FAULT_JUMP ||
				  reason == DASICS_FAULT_ECALL ? -EPERM : -EFAULT);
	if (ret)
		frame->fault.valid = false;
	return ret ?: DASICS_TRAP_TERMINAL;
}

int dasics_call_unwind_trap(struct pt_regs *regs)
{
	struct dasics_call_frame *frame;
	int ret;

	frame = this_cpu_read(dasics_active_call_frame);
	if (!frame || frame->owner != current ||
	    frame->magic != DASICS_CALL_FRAME_MAGIC ||
	    frame->state != DASICS_CALL_FRAME_FAULTED || !frame->fault.valid)
		return -EPROTO;

	ret = dasics_hw_redirect_recovery(regs, &frame->recovery,
					  &frame->parent_hw);
	if (ret) {
		dasics_call_fail_closed(frame, ret);
		return ret;
	}
	frame->recovery.error = frame->fault_error ?: -EFAULT;
	ret = dasics_hw_restore(&frame->parent_hw);
	if (ret) {
		dasics_hw_clear_call_authority();
	} else {
		frame->parent_saved = false;
	}
	return ret;
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
	ret = dasics_hw_call(regs, &frame->recovery);
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

int dasics_call_module_loader(struct module *module, void *target,
			      bool include_init, unsigned long *result)
{
#ifdef CONFIG_MODULES
	struct dasics_compartment compartment;
	struct dasics_call_policy policy = {
		.stack_size = DASICS_MODULE_STACK_SIZE,
	};
	struct dasics_call_regs regs = {};
	struct dasics_call_frame *frame;
	long ret;

	if (!module || !target)
		return -EINVAL;

	ret = dasics_compartment_init_loader(&compartment, module, target,
					     include_init);
	if (ret) {
		pr_err("%s: DASICS loader compartment rejected target %px: %ld\n",
		       module_name(module), target, ret);
		return ret;
	}

	frame = kzalloc(sizeof(*frame), GFP_KERNEL);
	if (!frame)
		return -ENOMEM;
	policy.callee = &compartment;
	policy.target = target;
	ret = dasics_call(frame, &policy, &regs);
	if (ret)
		pr_err("%s: DASICS loader call failed target=%px state=%u prepare=%d fault=%d finish=%d: %ld\n",
		       module_name(module), target, frame->state,
		       frame->prepare_error, frame->fault_error,
		       frame->finish_error, ret);
	if (!ret && result)
		*result = regs.ret_a0;
	kfree(frame);
	return ret;
#else
	return -EOPNOTSUPP;
#endif
}

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
