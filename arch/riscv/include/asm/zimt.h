/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_RISCV_ZIMT_H
#define _ASM_RISCV_ZIMT_H

#include <linux/stddef.h>
#include <linux/types.h>
#include <asm/page.h>
#include <asm/insn-def.h>

struct page;
struct pt_regs;

#ifdef CONFIG_RISCV_ISA_ZIMT

typedef u8 zimt_tag_t;

#define ZIMT_GRANULE_SIZE	16
#define ZIMT_GRANULES_PER_PAGE	(PAGE_SIZE / ZIMT_GRANULE_SIZE)

/*
 * VITT (Virtual Index Tag Table).
 *
 * Hardware maps data VA to tag VA as:  tag_va = vitt_base + (data_va >> 5)
 *
 * Kernel VITT (svitts): covers kernel image 0xffffffff80000000-0xffffffffffffffff
 * (2 GB), allocated via vmalloc at boot.  Size = 2GB / 32 = 64 MB.
 *
 * User VITT (svittu): per-process, dynamically mmap'd covering 0 to
 * mm->start_stack.  Size = start_stack / 32 (demand-paged).
 */
extern unsigned long zimt_kern_vitt_base;
#define ZIMT_KERN_VITT_SIZE	0x04000000UL	/* 64 MB */

int zimt_init_kern_vitt(void);
int zimt_setup_vitt(struct mm_struct *mm);

static inline unsigned long __zimt_hw_gentag(void)
{
	register unsigned long rd asm("a0");

	asm volatile(ZIMT_GENTAG(a0) : "=r" (rd));
	return rd;
}

static inline unsigned long __zimt_hw_addtag(unsigned long tagged_ptr, u8 imm4)
{
	register unsigned long rs1 asm("a0") = tagged_ptr;
	register unsigned long rd asm("a1");

	switch (imm4 & 0xf) {
	case 0:
		asm volatile(ZIMT_ADDTAG(a1, a0, 0) : "=r" (rd) : "r" (rs1));
		break;
	case 1:
		asm volatile(ZIMT_ADDTAG(a1, a0, 1) : "=r" (rd) : "r" (rs1));
		break;
	case 2:
		asm volatile(ZIMT_ADDTAG(a1, a0, 2) : "=r" (rd) : "r" (rs1));
		break;
	case 3:
		asm volatile(ZIMT_ADDTAG(a1, a0, 3) : "=r" (rd) : "r" (rs1));
		break;
	case 4:
		asm volatile(ZIMT_ADDTAG(a1, a0, 4) : "=r" (rd) : "r" (rs1));
		break;
	case 5:
		asm volatile(ZIMT_ADDTAG(a1, a0, 5) : "=r" (rd) : "r" (rs1));
		break;
	case 6:
		asm volatile(ZIMT_ADDTAG(a1, a0, 6) : "=r" (rd) : "r" (rs1));
		break;
	case 7:
		asm volatile(ZIMT_ADDTAG(a1, a0, 7) : "=r" (rd) : "r" (rs1));
		break;
	case 8:
		asm volatile(ZIMT_ADDTAG(a1, a0, 8) : "=r" (rd) : "r" (rs1));
		break;
	case 9:
		asm volatile(ZIMT_ADDTAG(a1, a0, 9) : "=r" (rd) : "r" (rs1));
		break;
	case 10:
		asm volatile(ZIMT_ADDTAG(a1, a0, 10) : "=r" (rd) : "r" (rs1));
		break;
	case 11:
		asm volatile(ZIMT_ADDTAG(a1, a0, 11) : "=r" (rd) : "r" (rs1));
		break;
	case 12:
		asm volatile(ZIMT_ADDTAG(a1, a0, 12) : "=r" (rd) : "r" (rs1));
		break;
	case 13:
		asm volatile(ZIMT_ADDTAG(a1, a0, 13) : "=r" (rd) : "r" (rs1));
		break;
	case 14:
		asm volatile(ZIMT_ADDTAG(a1, a0, 14) : "=r" (rd) : "r" (rs1));
		break;
	default:
		asm volatile(ZIMT_ADDTAG(a1, a0, 15) : "=r" (rd) : "r" (rs1));
		break;
	}

	return rd;
}

static inline void __zimt_hw_settag_1(unsigned long tagged_ptr)
{
	register unsigned long rs1 asm("a0") = tagged_ptr;

	asm volatile(ZIMT_SETTAG_1(a0) : : "r" (rs1) : "memory");
}

static inline void __zimt_hw_settag_16(unsigned long tagged_ptr)
{
	register unsigned long rs1 asm("a0") = tagged_ptr;

	asm volatile(ZIMT_SETTAG_16(a0) : : "r" (rs1) : "memory");
}

static inline void __zimt_hw_checktag_1(unsigned long tagged_ptr)
{
	register unsigned long rs1 asm("a0") = tagged_ptr;

	asm volatile(ZIMT_CHECKTAG_1(a0) : : "r" (rs1) : "memory");
}

bool zimt_hw_available(void);
bool zimt_check_tag(unsigned long addr);
zimt_tag_t zimt_get_tag(const void *addr);
void *zimt_set_tag(void *addr, zimt_tag_t tag);
zimt_tag_t zimt_generate_random_tag(u8 excl_mask);

void zimt_set_mem_tag(void *addr, zimt_tag_t tag, unsigned int count);
zimt_tag_t zimt_get_mem_tag(const void *addr);
void zimt_clear_page_tags(void *page_addr);
void zimt_save_page_tags(struct page *page);
void zimt_restore_page_tags(struct page *page);
void zimt_copy_page_tags(struct page *dst, struct page *src);
void zimt_invalidate_swap_tags(int type, pgoff_t offset);
void zimt_invalidate_swap_tags_area(int type);

void zimt_handle_tag_fault(struct pt_regs *regs);

void *zimt_kmalloc_post_alloc(void *ptr, size_t size, unsigned int gfp_flags);
void zimt_kfree_pre_free(void *ptr, size_t size);

#else /* !CONFIG_RISCV_ISA_ZIMT */

static inline bool zimt_hw_available(void) { return false; }
static inline bool zimt_check_tag(unsigned long addr) { return true; }
static inline void *zimt_kmalloc_post_alloc(void *ptr, size_t size, unsigned int gfp_flags)
{
	(void)size;
	(void)gfp_flags;
	return ptr;
}
static inline void zimt_kfree_pre_free(void *ptr, size_t size)
{
	(void)ptr;
	(void)size;
}
static inline void zimt_invalidate_swap_tags(int type, pgoff_t offset)
{
	(void)type;
	(void)offset;
}
static inline void zimt_invalidate_swap_tags_area(int type)
{
	(void)type;
}

#endif /* CONFIG_RISCV_ISA_ZIMT */

#endif /* _ASM_RISCV_ZIMT_H */
