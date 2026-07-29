/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _LINUX_DASICS_H
#define _LINUX_DASICS_H

#include <linux/bitops.h>
#include <linux/linkage.h>
#include <linux/types.h>

#include <asm/kdasics.h>

#define DASICS_POLICY_MAX_REGIONS 64
#define DASICS_DATA_BOUND_SLOTS 16
#define DASICS_JUMP_BOUND_SLOTS 4
#define DASICS_MAINCALL_STACK_SIZE 4096
#define DASICS_CALL_MAX_DEPTH 4
#define DASICS_COMPARTMENT_CODE_RANGES 2
#define DASICS_COMPARTMENT_DATA_RANGES 4
#define DASICS_MODULE_STACK_SIZE 4096

struct module;

typedef u64 dasics_bound_handle_t;

#define DASICS_BOUND_INVALID_HANDLE ((dasics_bound_handle_t)0)

enum dasics_bound_lifetime {
	DASICS_BOUND_LIFETIME_CALL = 1,
	DASICS_BOUND_LIFETIME_RUNTIME,
};

enum dasics_bound_flags {
	DASICS_BOUND_PINNED = BIT(0),
};

enum dasics_region_perm {
	DASICS_REGION_READ = BIT(0),
	DASICS_REGION_WRITE = BIT(1),
};

struct dasics_region {
	unsigned long base;
	size_t size;
	unsigned int perms;
};

struct dasics_code_range {
	unsigned long base;
	size_t size;
};

struct dasics_compartment {
	struct module *module;
	const struct dasics_code_range *code_ranges;
	unsigned int nr_code_ranges;
	const struct dasics_region *data_ranges;
	unsigned int nr_data_ranges;
	struct dasics_code_range
		loader_code_ranges[DASICS_COMPARTMENT_CODE_RANGES];
	struct dasics_region
		loader_data_ranges[DASICS_COMPARTMENT_DATA_RANGES];
	bool loader_managed;
	bool registered;
};

enum dasics_call_flags {
	DASICS_CALL_ALLOW_COMING = BIT(0),
};

struct dasics_call_policy {
	struct dasics_compartment *callee;
	void *target;
	const struct dasics_region *regions;
	unsigned int nr_regions;
	size_t stack_size;
	unsigned int flags;
};

struct dasics_bound_entry {
	dasics_bound_handle_t handle;
	unsigned long lo;
	unsigned long hi;
	unsigned int perms;
	enum dasics_bound_lifetime lifetime;
	unsigned int flags;
	int resident_slot;
	bool active;
};

/* Embedded in a trusted call frame; no metadata is allocated on a fault. */
struct dasics_bound_table {
	const struct dasics_compartment *owner;
	struct dasics_bound_entry entries[DASICS_POLICY_MAX_REGIONS];
	dasics_bound_handle_t slot_to_handle[DASICS_DATA_BOUND_SLOTS];
	unsigned int nr_entries;
	unsigned int nr_slots;
	unsigned int next_victim;
};

#define DASICS_CALL_FRAME_MAGIC 0x44415349U

enum dasics_call_frame_state {
	DASICS_CALL_FRAME_EMPTY,
	DASICS_CALL_FRAME_PREPARED,
	DASICS_CALL_FRAME_ENTERED,
	DASICS_CALL_FRAME_RETURNED,
	DASICS_CALL_FRAME_FAULTED,
	DASICS_CALL_FRAME_CLEANED,
};

enum dasics_fault_reason {
	DASICS_FAULT_ECALL = 1,
	DASICS_FAULT_LOAD,
	DASICS_FAULT_STORE,
	DASICS_FAULT_JUMP,
};

enum dasics_trap_result {
	DASICS_TRAP_TERMINAL,
	DASICS_TRAP_RETRY,
};

struct dasics_fault_record {
	unsigned long pc;
	unsigned long address;
	unsigned long reason;
	unsigned long cause;
	const struct dasics_compartment *compartment;
	bool valid;
};

/*
 * Call frames contain the complete software policy table and must live in
 * trusted preallocated storage, not in a function's kernel stack frame.
 */
struct dasics_call_frame {
	u32 magic;
	enum dasics_call_frame_state state;
	struct dasics_call_frame *parent;
	unsigned int depth;
	struct dasics_hw_state parent_hw;
	struct dasics_recovery_context recovery;
	struct dasics_maincall_request maincall_request;
	unsigned long maincall_stack[DASICS_MAINCALL_STACK_SIZE /
				     sizeof(unsigned long)] __aligned(16);
	struct dasics_bound_table data_bounds;
	struct dasics_fault_record fault;
	struct dasics_region normalized[DASICS_POLICY_MAX_REGIONS];
	const struct dasics_call_policy *policy;
	struct module *callee_module;
	dasics_bound_handle_t stack_handle;
	unsigned int nr_normalized;
	unsigned int nr_resident;
	unsigned int nr_jump_bounds;
	unsigned int data_misses;
	unsigned int data_refills;
	unsigned int test_fail_bound;
	unsigned int test_fail_restore;
	int prepare_error;
	int finish_error;
	int fault_error;
	int state_error;
	bool parent_saved;
	bool preempt_held;
	bool active_pushed;
	bool module_ref_held;
};

typedef int (*dasics_bound_clear_slot_fn)(unsigned int slot, void *context);

int dasics_policy_normalize(const struct dasics_call_policy *policy,
			    struct dasics_region *normalized,
			    unsigned int capacity,
			    unsigned int *nr_normalized);

int dasics_bound_table_init(struct dasics_bound_table *table,
			    const struct dasics_compartment *owner,
			    unsigned int nr_slots);
int dasics_bound_register(struct dasics_bound_table *table,
			  const struct dasics_region *region,
			  enum dasics_bound_lifetime lifetime,
			  unsigned int flags,
			  dasics_bound_handle_t *handle);
int dasics_bound_get(const struct dasics_bound_table *table,
		     dasics_bound_handle_t handle,
		     const struct dasics_bound_entry **entry);
int dasics_bound_find(const struct dasics_bound_table *table,
		      unsigned long address, unsigned int access,
		      dasics_bound_handle_t *handle);
int dasics_bound_select_slot(const struct dasics_bound_table *table,
			     dasics_bound_handle_t handle,
			     unsigned int *slot,
			     dasics_bound_handle_t *victim);
int dasics_bound_commit_resident(struct dasics_bound_table *table,
				 dasics_bound_handle_t handle,
				 unsigned int slot);
int dasics_bound_free(struct dasics_bound_table *table,
		      dasics_bound_handle_t handle,
		      dasics_bound_clear_slot_fn clear_slot, void *context);
int dasics_jump_policy_validate(unsigned int nr_jump_regions);

int dasics_compartment_init_module(struct dasics_compartment *compartment,
				   void *target);
void dasics_compartment_destroy(struct dasics_compartment *compartment);
int dasics_call_module_loader(struct module *module, void *target,
			      bool include_init, unsigned long *result);

int dasics_call_prepare(struct dasics_call_frame *frame,
			const struct dasics_call_policy *policy);
int dasics_call_recover(int error);
int dasics_call_handle_trap(unsigned long pc, unsigned long address,
			    unsigned long reason, unsigned long cause,
			    const struct dasics_compartment **compartment,
			    unsigned int *slot);
int dasics_call_unwind_trap(struct pt_regs *regs);
void dasics_call_finish(struct dasics_call_frame *frame);
long dasics_call(struct dasics_call_frame *frame,
		 const struct dasics_call_policy *policy,
		 struct dasics_call_regs *regs);
struct dasics_call_frame *dasics_call_current_frame(void);
asmlinkage struct dasics_maincall_request *dasics_maincall_dispatch(
		struct dasics_maincall_request *request);

#ifdef CONFIG_DASICS_DEBUG
typedef long (*dasics_maincall_debug_handler_t)(
		const struct dasics_maincall_request *request,
		unsigned long *value);

int dasics_maincall_debug_register(dasics_maincall_debug_handler_t handler);
void dasics_maincall_debug_unregister(
		dasics_maincall_debug_handler_t handler);
int dasics_call_test_fail_bound(struct dasics_call_frame *frame,
				unsigned int bound);
int dasics_call_test_fail_restore(struct dasics_call_frame *frame, bool fail);
int dasics_call_test_transition(struct dasics_call_frame *frame,
				enum dasics_call_frame_state next);
int dasics_call_test_handle_trap(unsigned long pc, unsigned long address,
				 unsigned long reason, unsigned long cause);
#endif

#endif /* _LINUX_DASICS_H */
