// SPDX-License-Identifier: GPL-2.0-only

#include <linux/dasics.h>
#include <linux/atomic.h>
#include <linux/errno.h>
#include <linux/export.h>
#include <linux/kernel.h>
#include <linux/limits.h>
#include <linux/string.h>

#define DASICS_REGION_PERMS (DASICS_REGION_READ | DASICS_REGION_WRITE)

static atomic64_t dasics_next_bound_handle = ATOMIC64_INIT(0);

static bool dasics_bound_table_valid(const struct dasics_bound_table *table)
{
	return table && table->owner && table->nr_slots &&
	       table->nr_slots <= DASICS_DATA_BOUND_SLOTS;
}

static int dasics_bound_alloc_handle(dasics_bound_handle_t *handle)
{
	u64 current;
	u64 next;

	do {
		current = atomic64_read(&dasics_next_bound_handle);
		if (current == U64_MAX)
			return -EOVERFLOW;
		next = current + 1;
	} while (atomic64_cmpxchg(&dasics_next_bound_handle, current, next) !=
		 current);

	*handle = next;
	return 0;
}

static int dasics_bound_entry_index(const struct dasics_bound_table *table,
				    dasics_bound_handle_t handle)
{
	unsigned int i;

	if (!dasics_bound_table_valid(table) ||
	    handle == DASICS_BOUND_INVALID_HANDLE)
		return -EINVAL;

	for (i = 0; i < ARRAY_SIZE(table->entries); i++) {
		if (table->entries[i].active &&
		    table->entries[i].handle == handle)
			return i;
	}

	return -ENOENT;
}

static int dasics_bound_resident_slot(const struct dasics_bound_table *table,
				      const struct dasics_bound_entry *entry)
{
	if (entry->resident_slot == -1)
		return -1;
	if (entry->resident_slot < -1 ||
	    entry->resident_slot >= table->nr_slots ||
	    table->slot_to_handle[entry->resident_slot] != entry->handle)
		return -EUCLEAN;

	return entry->resident_slot;
}

int dasics_bound_table_init(struct dasics_bound_table *table,
			    const struct dasics_compartment *owner,
			    unsigned int nr_slots)
{
	unsigned int i;

	if (!table || !owner || !nr_slots ||
	    nr_slots > DASICS_DATA_BOUND_SLOTS)
		return -EINVAL;

	memset(table, 0, sizeof(*table));
	table->owner = owner;
	table->nr_slots = nr_slots;
	for (i = 0; i < ARRAY_SIZE(table->entries); i++)
		table->entries[i].resident_slot = -1;

	return 0;
}
EXPORT_SYMBOL_GPL(dasics_bound_table_init);

int dasics_bound_register(struct dasics_bound_table *table,
			  const struct dasics_region *region,
			  enum dasics_bound_lifetime lifetime,
			  unsigned int flags,
			  dasics_bound_handle_t *handle)
{
	struct dasics_bound_entry *free_entry = NULL;
	unsigned long hi;
	unsigned int i;
	int ret;

	if (!dasics_bound_table_valid(table) || !region || !handle)
		return -EINVAL;
	*handle = DASICS_BOUND_INVALID_HANDLE;
	if (!region->size)
		return -EINVAL;
	if (region->size > ULONG_MAX - region->base)
		return -EOVERFLOW;
	if (!region->perms || (region->perms & ~DASICS_REGION_PERMS))
		return -EINVAL;
	if (lifetime != DASICS_BOUND_LIFETIME_CALL &&
	    lifetime != DASICS_BOUND_LIFETIME_RUNTIME)
		return -EINVAL;
	if (flags & ~DASICS_BOUND_PINNED)
		return -EINVAL;
	if (table->nr_entries >= ARRAY_SIZE(table->entries))
		return -EDQUOT;
	hi = region->base + region->size;
	for (i = 0; i < ARRAY_SIZE(table->entries); i++) {
		struct dasics_bound_entry *entry = &table->entries[i];

		if (!entry->active) {
			if (!free_entry)
				free_entry = entry;
			continue;
		}
		if (region->base < entry->hi && entry->lo < hi)
			return -EEXIST;
	}

	if (!free_entry)
		return -EDQUOT;

	ret = dasics_bound_alloc_handle(&free_entry->handle);
	if (ret)
		return ret;
	free_entry->lo = region->base;
	free_entry->hi = hi;
	free_entry->perms = region->perms;
	free_entry->lifetime = lifetime;
	free_entry->flags = flags;
	free_entry->resident_slot = -1;
	free_entry->active = true;
	table->nr_entries++;
	*handle = free_entry->handle;

	return 0;
}
EXPORT_SYMBOL_GPL(dasics_bound_register);

int dasics_bound_get(const struct dasics_bound_table *table,
		     dasics_bound_handle_t handle,
		     const struct dasics_bound_entry **entry)
{
	int index;

	if (!entry)
		return -EINVAL;
	*entry = NULL;
	index = dasics_bound_entry_index(table, handle);
	if (index < 0)
		return index;

	*entry = &table->entries[index];
	return 0;
}
EXPORT_SYMBOL_GPL(dasics_bound_get);

int dasics_bound_find(const struct dasics_bound_table *table,
		      unsigned long address, unsigned int access,
		      dasics_bound_handle_t *handle)
{
	unsigned int i;

	if (!dasics_bound_table_valid(table) || !handle || !access ||
	    (access & ~DASICS_REGION_PERMS))
		return -EINVAL;
	*handle = DASICS_BOUND_INVALID_HANDLE;

	for (i = 0; i < ARRAY_SIZE(table->entries); i++) {
		const struct dasics_bound_entry *entry = &table->entries[i];

		if (entry->active && address >= entry->lo &&
		    address < entry->hi &&
		    (entry->perms & access) == access) {
			*handle = entry->handle;
			return 0;
		}
	}

	return -ENOENT;
}
EXPORT_SYMBOL_GPL(dasics_bound_find);

int dasics_bound_select_slot(const struct dasics_bound_table *table,
			     dasics_bound_handle_t handle,
			     unsigned int *slot,
			     dasics_bound_handle_t *victim)
{
	const struct dasics_bound_entry *entry;
	unsigned int i;
	int ret;

	if (!slot || !victim)
		return -EINVAL;
	*victim = DASICS_BOUND_INVALID_HANDLE;
	ret = dasics_bound_get(table, handle, &entry);
	if (ret)
		return ret;
	ret = dasics_bound_resident_slot(table, entry);
	if (ret >= 0) {
		*slot = ret;
		return 0;
	}
	if (ret != -1)
		return ret;

	for (i = 0; i < table->nr_slots; i++) {
		unsigned int candidate = (table->next_victim + i) %
					 table->nr_slots;

		if (table->slot_to_handle[candidate] ==
		    DASICS_BOUND_INVALID_HANDLE) {
			*slot = candidate;
			return 0;
		}
	}

	for (i = 0; i < table->nr_slots; i++) {
		const struct dasics_bound_entry *resident;
		unsigned int candidate = (table->next_victim + i) %
					 table->nr_slots;
		dasics_bound_handle_t resident_handle =
			table->slot_to_handle[candidate];

		ret = dasics_bound_get(table, resident_handle, &resident);
		if (ret)
			return -EUCLEAN;
		if (resident->resident_slot != candidate)
			return -EUCLEAN;
		if (!(resident->flags & DASICS_BOUND_PINNED)) {
			*slot = candidate;
			*victim = resident_handle;
			return 0;
		}
	}

	return -ENOSPC;
}
EXPORT_SYMBOL_GPL(dasics_bound_select_slot);

int dasics_bound_commit_resident(struct dasics_bound_table *table,
				 dasics_bound_handle_t handle,
				 unsigned int slot)
{
	struct dasics_bound_entry *entry;
	dasics_bound_handle_t old_handle;
	int index;
	int old_index;

	if (!dasics_bound_table_valid(table) || slot >= table->nr_slots)
		return -EINVAL;
	index = dasics_bound_entry_index(table, handle);
	if (index < 0)
		return index;
	entry = &table->entries[index];
	old_index = dasics_bound_resident_slot(table, entry);
	if (old_index >= 0)
		return old_index == slot ? 0 : -EALREADY;
	if (old_index != -1)
		return old_index;

	old_handle = table->slot_to_handle[slot];
	if (old_handle != DASICS_BOUND_INVALID_HANDLE) {
		old_index = dasics_bound_entry_index(table, old_handle);
		if (old_index < 0)
			return -EUCLEAN;
		if (table->entries[old_index].resident_slot != slot)
			return -EUCLEAN;
		if (table->entries[old_index].flags & DASICS_BOUND_PINNED)
			return -EBUSY;
		table->entries[old_index].resident_slot = -1;
	}

	table->slot_to_handle[slot] = handle;
	entry->resident_slot = slot;
	table->next_victim = (slot + 1) % table->nr_slots;

	return 0;
}
EXPORT_SYMBOL_GPL(dasics_bound_commit_resident);

int dasics_bound_free(struct dasics_bound_table *table,
		      dasics_bound_handle_t handle,
		      dasics_bound_clear_slot_fn clear_slot, void *context)
{
	struct dasics_bound_entry *entry;
	int index;
	int ret;

	index = dasics_bound_entry_index(table, handle);
	if (index < 0)
		return index;
	entry = &table->entries[index];

	ret = dasics_bound_resident_slot(table, entry);
	if (ret >= 0) {
		unsigned int slot = ret;

		if (!clear_slot)
			return -EINVAL;
		ret = clear_slot(slot, context);
		if (ret)
			return ret;
		table->slot_to_handle[slot] = DASICS_BOUND_INVALID_HANDLE;
	} else if (ret != -1) {
		return ret;
	}

	memset(entry, 0, sizeof(*entry));
	entry->resident_slot = -1;
	table->nr_entries--;
	return 0;
}
EXPORT_SYMBOL_GPL(dasics_bound_free);

int dasics_jump_policy_validate(unsigned int nr_jump_regions)
{
	return nr_jump_regions <= DASICS_JUMP_BOUND_SLOTS ? 0 : -E2BIG;
}
EXPORT_SYMBOL_GPL(dasics_jump_policy_validate);
