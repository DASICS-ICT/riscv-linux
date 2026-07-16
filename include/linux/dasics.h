/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _LINUX_DASICS_H
#define _LINUX_DASICS_H

#include <linux/bitops.h>
#include <linux/types.h>

#include <asm/kdasics.h>

#define DASICS_POLICY_MAX_REGIONS 64
#define DASICS_DATA_BOUND_SLOTS 16
#define DASICS_JUMP_BOUND_SLOTS 4

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
	struct dasics_code_range loader_code_ranges[2];
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
	DASICS_CALL_FRAME_IDLE,
	DASICS_CALL_FRAME_PREPARED,
	DASICS_CALL_FRAME_FINISHED,
};

/*
 * Call frames contain the complete software policy table and must live in
 * trusted preallocated storage, not in a function's kernel stack frame.
 */
struct dasics_call_frame {
	u32 magic;
	enum dasics_call_frame_state state;
	struct dasics_hw_state parent_hw;
	struct dasics_bound_table data_bounds;
	struct dasics_region normalized[DASICS_POLICY_MAX_REGIONS];
	const struct dasics_call_policy *policy;
	dasics_bound_handle_t stack_handle;
	unsigned int nr_normalized;
	unsigned int nr_resident;
	unsigned int nr_jump_bounds;
	unsigned int test_fail_bound;
	unsigned int test_fail_restore;
	int prepare_error;
	int finish_error;
	bool parent_saved;
	bool preempt_held;
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

int dasics_call_prepare(struct dasics_call_frame *frame,
			const struct dasics_call_policy *policy);
void dasics_call_finish(struct dasics_call_frame *frame);
long dasics_call(struct dasics_call_frame *frame,
		 const struct dasics_call_policy *policy,
		 struct dasics_call_regs *regs);

#ifdef CONFIG_DASICS_DEBUG
int dasics_call_test_fail_bound(struct dasics_call_frame *frame,
				unsigned int bound);
int dasics_call_test_fail_restore(struct dasics_call_frame *frame, bool fail);
#endif

#endif /* _LINUX_DASICS_H */
