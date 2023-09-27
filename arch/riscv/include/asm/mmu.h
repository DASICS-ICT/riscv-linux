/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2012 Regents of the University of California
 */


#ifndef _ASM_RISCV_MMU_H
#define _ASM_RISCV_MMU_H

#ifndef __ASSEMBLY__

typedef struct {
#ifndef CONFIG_MMU
	unsigned long	end_brk;
#endif
	void *vdso;
#ifdef CONFIG_SMP
	/* A local icache flush is needed before user execution can resume. */
	cpumask_t icache_stale_mask;
#endif
#ifdef CONFIG_RISCV_MEMORY_PROTECTION_KEYS
	/*
	 * One bit per protection key says whether userspace can
	 * use it or not.  protected by mmap_sem.
	 */
	u32 pkey_allocation_map;
	s32 execute_only_pkey;
#endif  /* CONFIG_RISCV_MEMORY_PROTECTION_KEYS */
} mm_context_t;

void __init create_pgd_mapping(pgd_t *pgdp, uintptr_t va, phys_addr_t pa,
			       phys_addr_t sz, pgprot_t prot);
#endif /* __ASSEMBLY__ */

#endif /* _ASM_RISCV_MMU_H */
