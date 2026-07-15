/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _LINUX_DASICS_H
#define _LINUX_DASICS_H

#include <linux/bitops.h>
#include <linux/types.h>

#define DASICS_POLICY_MAX_REGIONS 64
#define DASICS_DATA_BOUND_SLOTS 16
#define DASICS_JUMP_BOUND_SLOTS 4

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

/* C3 only needs immutable executable ranges; lifecycle fields come later. */
struct dasics_compartment {
	const struct dasics_code_range *code_ranges;
	unsigned int nr_code_ranges;
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

#endif /* _LINUX_DASICS_H */
