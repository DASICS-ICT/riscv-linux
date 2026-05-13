// SPDX-License-Identifier: GPL-2.0-only
/*
 * RISC-V ZIMT (tagged memory) — software helpers. Hardware uses gentag/settag/checktag.
 */

#include <linux/bitops.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/module.h>
#include <linux/sched/signal.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/xarray.h>
#include <asm/hwcap.h>
#include <asm/ptrace.h>
#include <asm/uaccess.h>
#include <asm/zimt.h>
#ifdef CONFIG_DASICS
#include <asm/kdasics.h>
#endif

#ifdef CONFIG_RISCV_ISA_ZIMT

unsigned long zimt_kern_vitt_base __read_mostly;

int zimt_init_kern_vitt(void)
{
	void *area;

	if (zimt_kern_vitt_base)
		return 0;

	area = vzalloc(ZIMT_KERN_VITT_SIZE);
	if (!area) {
		pr_err("ZIMT: failed to allocate kernel VITT (%lu MB)\n",
		       ZIMT_KERN_VITT_SIZE >> 20);
		return -ENOMEM;
	}

	zimt_kern_vitt_base = (unsigned long)area;
	pr_info("ZIMT: kernel VITT at 0x%lx (size 0x%lx)\n",
		zimt_kern_vitt_base, (unsigned long)ZIMT_KERN_VITT_SIZE);
	return 0;
}

static DEFINE_XARRAY(zimt_tag_shadow);

static inline unsigned int zimt_page_granule_offset(unsigned long addr)
{
	return (addr & ~PAGE_MASK) / ZIMT_GRANULE_SIZE;
}

static struct page *zimt_addr_to_page(unsigned long addr)
{
	if (!virt_addr_valid((void *)addr))
		return NULL;

	return virt_to_page((void *)addr);
}

static u8 *zimt_get_shadow(struct page *page)
{
	return xa_load(&zimt_tag_shadow, page_to_pfn(page));
}

static u8 *zimt_get_or_alloc_shadow(struct page *page)
{
	unsigned long index = page_to_pfn(page);
	u8 *shadow = xa_load(&zimt_tag_shadow, index);

	if (shadow)
		return shadow;

	shadow = kzalloc(ZIMT_GRANULES_PER_PAGE, GFP_KERNEL);
	if (!shadow)
		return NULL;

	if (xa_cmpxchg(&zimt_tag_shadow, index, NULL, shadow, GFP_KERNEL)) {
		kfree(shadow);
		shadow = xa_load(&zimt_tag_shadow, index);
	}

	return shadow;
}

bool zimt_hw_available(void)
{
	return riscv_has_extension_unlikely(RISCV_ISA_EXT_ZIMT);
}

bool zimt_check_tag(unsigned long addr)
{
	if (!zimt_hw_available())
		return true;

	/*
	 * Execute checktag directly. Hardware raises software-check trap on
	 * mismatch, so successful return means the tag check passed.
	 */
	__zimt_hw_checktag_1(addr);
	return true;
}
EXPORT_SYMBOL_GPL(zimt_hw_available);
EXPORT_SYMBOL_GPL(zimt_check_tag);

zimt_tag_t zimt_get_tag(const void *addr)
{
	unsigned long pmlen = 16;

#ifdef CONFIG_RISCV_ISA_SUPM
	if (current->mm)
		pmlen = current->mm->context.pmlen;
#endif
	return (zimt_tag_t)(((unsigned long)addr >> (64 - pmlen)) & 0xff);
}

void *zimt_set_tag(void *addr, zimt_tag_t tag)
{
	unsigned long pmlen = 16;
	unsigned long a;

#ifdef CONFIG_RISCV_ISA_SUPM
	if (current->mm)
		pmlen = current->mm->context.pmlen;
#endif
	a = (unsigned long)untagged_addr(addr);

	if (zimt_hw_available() && pmlen == 16)
		return (void *)__zimt_hw_addtag(a, tag & 0xf);

	return (void *)(a | ((unsigned long)tag << (64 - pmlen)));
}

zimt_tag_t zimt_generate_random_tag(u8 excl_mask)
{
	u8 tag;
	int retries = 32;

	if (!zimt_hw_available())
		return 0;

	do {
		unsigned long tagged = __zimt_hw_gentag();

		tag = zimt_get_tag((void *)tagged);
		if (tag >= 8 || !(excl_mask & BIT(tag)))
			return tag;
	} while (--retries > 0);

	return tag;
}

void zimt_set_mem_tag(void *addr, zimt_tag_t tag, unsigned int count)
{
	unsigned long base = (unsigned long)untagged_addr(addr);
	unsigned long tagged_ptr;
	struct page *page;
	u8 *shadow;
	unsigned int i, off, total = count;

	if (!count)
		return;

	tagged_ptr = (unsigned long)zimt_set_tag((void *)base, tag);

	if (zimt_hw_available()) {
		while (count >= 16) {
			__zimt_hw_settag_16(tagged_ptr);
			tagged_ptr += ZIMT_GRANULE_SIZE * 16;
			count -= 16;
		}
		while (count--) {
			__zimt_hw_settag_1(tagged_ptr);
			tagged_ptr += ZIMT_GRANULE_SIZE;
		}
	}

	page = zimt_addr_to_page(base);
	if (!page)
		return;

	shadow = zimt_get_or_alloc_shadow(page);
	if (!shadow)
		return;

	off = zimt_page_granule_offset(base);
	if (off >= ZIMT_GRANULES_PER_PAGE)
		return;

	total = min(total, ZIMT_GRANULES_PER_PAGE - off);
	for (i = 0; i < total; i++)
		shadow[off + i] = tag;
}

zimt_tag_t zimt_get_mem_tag(const void *addr)
{
	unsigned long base = (unsigned long)untagged_addr(addr);
	struct page *page = zimt_addr_to_page(base);
	u8 *shadow;
	unsigned int off;

	if (!page)
		return 0;

	shadow = zimt_get_shadow(page);
	if (!shadow)
		return 0;

	off = zimt_page_granule_offset(base);
	if (off >= ZIMT_GRANULES_PER_PAGE)
		return 0;

	return shadow[off];
}

void zimt_clear_page_tags(void *page_addr)
{
	unsigned long ptr = (unsigned long)page_addr;
	struct page *page = NULL;
	u8 *shadow = NULL;
	unsigned int i;

	if (virt_addr_valid(page_addr)) {
		page = virt_to_page(page_addr);
		shadow = zimt_get_shadow(page);
	}

	if (zimt_hw_available()) {
		for (i = 0; i < ZIMT_GRANULES_PER_PAGE / 16; i++) {
			__zimt_hw_settag_16(ptr);
			ptr += ZIMT_GRANULE_SIZE * 16;
		}
	}

	if (shadow)
		memset(shadow, 0, ZIMT_GRANULES_PER_PAGE);
}

void zimt_save_page_tags(struct page *page)
{
	/*
	 * Hardware does not provide a direct "read tag memory" operation.
	 * We keep tags in a software shadow table as they are written.
	 */
	(void)zimt_get_or_alloc_shadow(page);
}

void zimt_restore_page_tags(struct page *page)
{
	u8 *shadow = zimt_get_shadow(page);
	unsigned long base;
	unsigned int i;

	if (!shadow || !zimt_hw_available())
		return;

	base = (unsigned long)page_address(page);
	for (i = 0; i < ZIMT_GRANULES_PER_PAGE; i++)
		zimt_set_mem_tag((void *)(base + i * ZIMT_GRANULE_SIZE),
				 shadow[i], 1);
}

void zimt_copy_page_tags(struct page *dst, struct page *src)
{
	u8 *src_shadow = zimt_get_shadow(src);
	u8 *dst_shadow;
	unsigned long base;
	unsigned int i;

	if (!src_shadow)
		return;

	dst_shadow = zimt_get_or_alloc_shadow(dst);
	if (!dst_shadow)
		return;

	memcpy(dst_shadow, src_shadow, ZIMT_GRANULES_PER_PAGE);

	if (!zimt_hw_available())
		return;

	base = (unsigned long)page_address(dst);
	for (i = 0; i < ZIMT_GRANULES_PER_PAGE; i++)
		zimt_set_mem_tag((void *)(base + i * ZIMT_GRANULE_SIZE),
				 dst_shadow[i], 1);
}

void zimt_handle_tag_fault(struct pt_regs *regs)
{
	unsigned long addr = regs->badaddr;

	force_sig_fault(SIGSEGV, SEGV_MTESERR, (void __user *)addr);
}

void *zimt_kmalloc_post_alloc(void *ptr, size_t size, unsigned int gfp_flags)
{
	unsigned long base;
	unsigned int granules, max_granules;
	zimt_tag_t tag;

	(void)gfp_flags;

	if (!ptr || !size || !zimt_hw_available())
		return ptr;

	base = (unsigned long)untagged_addr(ptr);
	max_granules = ZIMT_GRANULES_PER_PAGE - zimt_page_granule_offset(base);
	granules = min_t(unsigned int, DIV_ROUND_UP(size, ZIMT_GRANULE_SIZE), max_granules);

	tag = zimt_generate_random_tag(0);
	zimt_set_mem_tag((void *)base, tag, granules);

	return zimt_set_tag((void *)base, tag);
}

void zimt_kfree_pre_free(void *ptr, size_t size)
{
	unsigned long base;
	unsigned int granules, max_granules;
	zimt_tag_t ptr_tag, mem_tag;

	if (!ptr || !size || !zimt_hw_available())
		return;

	base = (unsigned long)untagged_addr(ptr);
	ptr_tag = zimt_get_tag(ptr);
	mem_tag = zimt_get_mem_tag((void *)base);
	if (ptr_tag != mem_tag)
		pr_warn_ratelimited("ZIMT: free tag mismatch ptr=%u mem=%u addr=%px\n",
				    ptr_tag, mem_tag, ptr);

	max_granules = ZIMT_GRANULES_PER_PAGE - zimt_page_granule_offset(base);
	granules = min_t(unsigned int, DIV_ROUND_UP(size, ZIMT_GRANULE_SIZE), max_granules);
	zimt_set_mem_tag((void *)base, 0, granules);
}

/*
 * Set up user-mode VITT (svittu) for the current process.
 *
 * The VITT must cover the entire user virtual address space [0, TASK_SIZE)
 * because QEMU's hardware VITT protection blocks normal user accesses to
 * [vitt_base, vitt_base + (max_user_va >> 5)].  If the VITT allocation
 * is smaller than this protection range, libraries mapped in the gap
 * between the VITT VMA end and the protection end become inaccessible.
 *
 * For SV39 (TASK_SIZE = 256 GB):  vitt_size = 256GB / 32 = 8 GB.
 * Pages are demand-allocated (MAP_ANONYMOUS | MAP_NORESERVE), so physical
 * memory is consumed only when tags are actually written.
 */
#ifdef CONFIG_DASICS
static unsigned long zimt_mmap_vitt_away_from_libbounds(unsigned long vitt_size,
							struct dasics_libbound *bounds,
							int nr_bounds)
{
	unsigned long addr = (TASK_SIZE - vitt_size) & PAGE_MASK;
	unsigned long err = -ENOMEM;

	while (addr >= PAGE_SIZE) {
		bool adjusted = false;

		for (int i = 0; i < nr_bounds; ++i) {
			if (addr >= bounds[i].hi || addr + vitt_size <= bounds[i].lo)
				continue;
			if (bounds[i].lo <= vitt_size)
				return -ENOMEM;
			addr = (bounds[i].lo - vitt_size) & PAGE_MASK;
			adjusted = true;
			break;
		}

		if (adjusted)
			continue;

		err = vm_mmap(NULL, addr, vitt_size,
			      PROT_READ | PROT_WRITE,
			      MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE |
			      MAP_FIXED_NOREPLACE, 0);
		if (!IS_ERR_VALUE(err))
			return err;
		if ((long)err != -EEXIST)
			return err;
		addr -= PAGE_SIZE;
	}

	return err;
}
#endif

int zimt_setup_vitt(struct mm_struct *mm)
{
	unsigned long vitt_size, addr;
#ifdef CONFIG_DASICS
	struct dasics_libbound bounds[DASICS_LIBCFG_WIDTH];
	int nr_bounds;
#endif

	if (current->thread.zimt_vitt_base)
		return 0;

	vitt_size = TASK_SIZE >> 5;
	if (!vitt_size)
		return -EINVAL;

#ifdef CONFIG_DASICS
	nr_bounds = dasics_get_libbounds(current, bounds, DASICS_LIBCFG_WIDTH);
	if (nr_bounds > 0)
		addr = zimt_mmap_vitt_away_from_libbounds(vitt_size, bounds,
							  nr_bounds);
	else
#endif
		addr = vm_mmap(NULL, 0, vitt_size,
			       PROT_READ | PROT_WRITE,
			       MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, 0);
	if (IS_ERR_VALUE(addr)) {
		pr_warn("ZIMT: user VITT mmap failed (size 0x%lx, err %ld)\n",
			vitt_size, (long)addr);
		return -ENOMEM;
	}

	current->thread.zimt_vitt_base = addr;
	csr_write(CSR_SVITTU, addr);

	pr_info("ZIMT: user VITT at 0x%lx (size 0x%lx, covers 0-0x%lx)\n",
		addr, vitt_size, (unsigned long)TASK_SIZE);
	return 0;
}

#endif /* CONFIG_RISCV_ISA_ZIMT */
