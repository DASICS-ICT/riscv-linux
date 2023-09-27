/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2012 Regents of the University of California
 */

#ifndef _ASM_RISCV_PGTABLE_64_H
#define _ASM_RISCV_PGTABLE_64_H

#include <linux/bitops.h>
#include <linux/const.h>

#define PGDIR_SHIFT     30
/* Size of region mapped by a page global directory */
#define PGDIR_SIZE      (_AC(1, UL) << PGDIR_SHIFT)
#define PGDIR_MASK      (~(PGDIR_SIZE - 1))

#define PMD_SHIFT       21
/* Size of region mapped by a page middle directory */
#define PMD_SIZE        (_AC(1, UL) << PMD_SHIFT)
#define PMD_MASK        (~(PMD_SIZE - 1))

/* Page Middle Directory entry */
typedef struct {
	unsigned long pmd;
} pmd_t;

#define pmd_val(x)      ((x).pmd)
#define __pmd(x)        ((pmd_t) { (x) })

#define PTRS_PER_PMD    (PAGE_SIZE / sizeof(pmd_t))

/*
 * rv64 PTE format:
 * | 63 | 62 61 | 60 59 | 58 54 | 53  10 | 9             8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0
 *   N      MT     RSV     PKEY     PFN    reserved for SW   D   A   G   U   X   W   R   V
 */
#define _PAGE_PFN_MASK  GENMASK(53, 10)

#ifdef CONFIG_RISCV_MEMORY_PROTECTION_KEYS
#define _PAGE_PKEY_BIT0 (1UL << 54)
#define _PAGE_PKEY_BIT1 (1UL << 55)
#define _PAGE_PKEY_BIT2 (1UL << 56)
#define _PAGE_PKEY_BIT3 (1UL << 57)
#define _PAGE_PKEY_BIT4 (1UL << 58)
#define _PAGE_PKEY_MASK (_PAGE_PKEY_BIT0 | _PAGE_PKEY_BIT1 | \
						 _PAGE_PKEY_BIT2 | _PAGE_PKEY_BIT3 | \
						 _PAGE_PKEY_BIT4)
#endif /* CONFIG_RISCV_MEMORY_PROTECTION_KEYS */

static inline int pud_present(pud_t pud)
{
	return (pud_val(pud) & _PAGE_PRESENT);
}

static inline int pud_none(pud_t pud)
{
	return (pud_val(pud) == 0);
}

static inline int pud_bad(pud_t pud)
{
	return !pud_present(pud);
}

#define pud_leaf	pud_leaf
static inline int pud_leaf(pud_t pud)
{
	return pud_present(pud) &&
	       (pud_val(pud) & (_PAGE_READ | _PAGE_WRITE | _PAGE_EXEC));
}

static inline void set_pud(pud_t *pudp, pud_t pud)
{
	*pudp = pud;
}

static inline void pud_clear(pud_t *pudp)
{
	set_pud(pudp, __pud(0));
}

static inline unsigned long pud_page_vaddr(pud_t pud)
{
	return (unsigned long)pfn_to_virt(__page_val_to_pfn(pud_val(pud)));
}

static inline struct page *pud_page(pud_t pud)
{
	return pfn_to_page(pud_val(pud) >> _PAGE_PFN_SHIFT);
}

static inline pmd_t pfn_pmd(unsigned long pfn, pgprot_t prot)
{
	return __pmd((pfn << _PAGE_PFN_SHIFT) | pgprot_val(prot));
}

static inline unsigned long _pmd_pfn(pmd_t pmd)
{
	return pmd_val(pmd) >> _PAGE_PFN_SHIFT;
}

#define pmd_ERROR(e) \
	pr_err("%s:%d: bad pmd %016lx.\n", __FILE__, __LINE__, pmd_val(e))

#endif /* _ASM_RISCV_PGTABLE_64_H */
