/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _LINUX_DASICS_H
#define _LINUX_DASICS_H

#include <linux/bitops.h>
#include <linux/types.h>

#define DASICS_POLICY_MAX_REGIONS 64

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

int dasics_policy_normalize(const struct dasics_call_policy *policy,
			    struct dasics_region *normalized,
			    unsigned int capacity,
			    unsigned int *nr_normalized);

#endif /* _LINUX_DASICS_H */
