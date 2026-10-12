// SPDX-License-Identifier: GPL-2.0-only

#include <linux/dasics.h>
#include <linux/errno.h>
#include <linux/export.h>
#include <linux/kernel.h>
#include <linux/limits.h>

#define DASICS_REGION_PERMS (DASICS_REGION_READ | DASICS_REGION_WRITE)

static int dasics_range_end(unsigned long base, size_t size,
			    unsigned long *end)
{
	if (!size)
		return -EINVAL;
	if (size > ULONG_MAX - base)
		return -EOVERFLOW;

	*end = base + size;
	return 0;
}

static int dasics_policy_validate_target(const struct dasics_call_policy *policy)
{
	struct dasics_compartment *callee;
	unsigned long target = (unsigned long)policy->target;
	unsigned int i;
	bool found = false;

	callee = policy->callee;
	if (!callee || !callee->code_ranges || !callee->nr_code_ranges ||
	    !target)
		return -EINVAL;

	for (i = 0; i < callee->nr_code_ranges; i++) {
		const struct dasics_code_range *range = &callee->code_ranges[i];
		unsigned long end;
		int ret;

		ret = dasics_range_end(range->base, range->size, &end);
		if (ret)
			return ret;
		if (target >= range->base && target < end)
			found = true;
	}

	return found ? 0 : -EPERM;
}

static int dasics_region_compare(const struct dasics_region *left,
				 const struct dasics_region *right)
{
	if (left->base < right->base)
		return -1;
	if (left->base > right->base)
		return 1;
	if (left->size < right->size)
		return -1;
	if (left->size > right->size)
		return 1;
	if (left->perms < right->perms)
		return -1;
	if (left->perms > right->perms)
		return 1;
	return 0;
}

static void dasics_regions_sort(struct dasics_region *regions,
				unsigned int nr_regions)
{
	unsigned int i;

	for (i = 1; i < nr_regions; i++) {
		struct dasics_region current = regions[i];
		unsigned int j = i;

		while (j && dasics_region_compare(&current, &regions[j - 1]) < 0) {
			regions[j] = regions[j - 1];
			j--;
		}
		regions[j] = current;
	}
}

static int dasics_regions_merge(struct dasics_region *regions,
				unsigned int nr_regions,
				unsigned int *nr_merged)
{
	unsigned int input;
	unsigned int output = 0;

	for (input = 0; input < nr_regions; input++) {
		struct dasics_region *current = &regions[input];
		unsigned long current_end;
		int ret;

		ret = dasics_range_end(current->base, current->size, &current_end);
		if (ret)
			return ret;
		if (!current->perms || (current->perms & ~DASICS_REGION_PERMS))
			return -EINVAL;

		if (output) {
			struct dasics_region *previous = &regions[output - 1];
			unsigned long previous_end = previous->base + previous->size;

			if (current->base < previous_end &&
			    current->perms != previous->perms)
				return -EINVAL;
			if (current->perms == previous->perms &&
			    current->base <= previous_end) {
				if (current_end > previous_end)
					previous->size = current_end - previous->base;
				continue;
			}
		}

		regions[output++] = *current;
	}

	*nr_merged = output;
	return 0;
}

int dasics_policy_normalize(const struct dasics_call_policy *policy,
			    struct dasics_region *normalized,
			    unsigned int capacity,
			    unsigned int *nr_normalized)
{
	unsigned int nr_merged;
	unsigned int i;
	int ret;

	if (!policy || !nr_normalized)
		return -EINVAL;
	*nr_normalized = 0;

	ret = dasics_policy_validate_target(policy);
	if (ret)
		return ret;
	if (policy->nr_regions && (!policy->regions || !normalized))
		return -EINVAL;
	if (capacity < policy->nr_regions)
		return -ENOSPC;

	for (i = 0; i < policy->nr_regions; i++)
		normalized[i] = policy->regions[i];

	dasics_regions_sort(normalized, policy->nr_regions);
	ret = dasics_regions_merge(normalized, policy->nr_regions, &nr_merged);
	if (ret)
		return ret;
	if (nr_merged > DASICS_POLICY_MAX_REGIONS)
		return -E2BIG;

	*nr_normalized = nr_merged;
	return 0;
}
EXPORT_SYMBOL_GPL(dasics_policy_normalize);
