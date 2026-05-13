// SPDX-License-Identifier: GPL-2.0-only

#include <linux/export.h>
#include <linux/highmem.h>
#include <linux/mm.h>
#include <asm/page.h>
#ifdef CONFIG_RISCV_ISA_ZIMT
#include <asm/zimt.h>
#endif

void copy_highpage(struct page *to, struct page *from)
{
	void *kto = page_address(to);
	void *kfrom = page_address(from);

	copy_page(kto, kfrom);

#ifdef CONFIG_RISCV_ISA_ZIMT
	if (!zimt_hw_available())
		return;

	zimt_copy_page_tags(to, from);
#endif
}
EXPORT_SYMBOL(copy_highpage);
