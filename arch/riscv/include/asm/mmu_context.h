/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2012 Regents of the University of California
 * Copyright (C) 2017 SiFive
 */

#ifndef _ASM_RISCV_MMU_CONTEXT_H
#define _ASM_RISCV_MMU_CONTEXT_H

#include <linux/mm_types.h>
#include <asm-generic/mm_hooks.h>

#include <linux/mm.h>
#include <linux/sched.h>

#include <linux/pkeys.h>
#include <asm/tlbflush.h>
#include <asm/cacheflush.h>

static inline void enter_lazy_tlb(struct mm_struct *mm,
	struct task_struct *task)
{
}

/* Initialize context-related info for a new mm_struct */
static inline int init_new_context(struct task_struct *task,
	struct mm_struct *mm)
{
#ifdef CONFIG_RISCV_MEMORY_PROTECTION_KEYS
	if (arch_pkeys_enabled()) {
		/* pkey 0 is the default and allocated implicitly */
		mm->context.pkey_allocation_map = 0x1;
		/* -1 means unallocated or invalid */
		mm->context.execute_only_pkey = -1;
	}
#endif
	return 0;
}

static inline void destroy_context(struct mm_struct *mm)
{
}

void switch_mm(struct mm_struct *prev, struct mm_struct *next,
	struct task_struct *task);

static inline void activate_mm(struct mm_struct *prev,
			       struct mm_struct *next)
{
	switch_mm(prev, next, NULL);
}

static inline void deactivate_mm(struct task_struct *task,
	struct mm_struct *mm)
{
}

#endif /* _ASM_RISCV_MMU_CONTEXT_H */
