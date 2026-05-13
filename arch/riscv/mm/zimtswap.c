// SPDX-License-Identifier: GPL-2.0-only

#include <linux/pagemap.h>
#include <linux/slab.h>
#include <linux/swap.h>
#include <linux/swapops.h>
#include <linux/xarray.h>
#include <asm/zimt.h>

static DEFINE_XARRAY(zimt_swap_pages);

static void *zimt_allocate_tag_storage(void)
{
	return kmalloc(ZIMT_GRANULES_PER_PAGE, GFP_KERNEL);
}

static void zimt_free_tag_storage(void *storage)
{
	kfree(storage);
}

static int zimt_save_tags_to_swap(swp_entry_t entry, struct page *page)
{
	char *page_addr = page_address(page);
	u8 *tag_storage, *old;
	unsigned int i;

	if (!page_addr)
		return -EINVAL;

	tag_storage = zimt_allocate_tag_storage();
	if (!tag_storage)
		return -ENOMEM;

	for (i = 0; i < ZIMT_GRANULES_PER_PAGE; i++)
		tag_storage[i] = zimt_get_mem_tag(page_addr + i * ZIMT_GRANULE_SIZE);

	old = xa_store(&zimt_swap_pages, entry.val, tag_storage, GFP_KERNEL);
	if (xa_is_err(old)) {
		zimt_free_tag_storage(tag_storage);
		return xa_err(old);
	}

	if (old)
		zimt_free_tag_storage(old);

	return 0;
}

static void zimt_restore_tags_from_swap(swp_entry_t entry, struct page *page)
{
	char *page_addr = page_address(page);
	u8 *tag_storage = xa_load(&zimt_swap_pages, entry.val);
	unsigned int i;

	if (!tag_storage || !page_addr)
		return;

	for (i = 0; i < ZIMT_GRANULES_PER_PAGE; i++)
		zimt_set_mem_tag(page_addr + i * ZIMT_GRANULE_SIZE, tag_storage[i], 1);
}

void zimt_invalidate_swap_tags(int type, pgoff_t offset)
{
	swp_entry_t entry = swp_entry(type, offset);
	void *tags = xa_erase(&zimt_swap_pages, entry.val);

	zimt_free_tag_storage(tags);
}

void zimt_invalidate_swap_tags_area(int type)
{
	swp_entry_t entry = swp_entry(type, 0);
	swp_entry_t last_entry = swp_entry(type + 1, 0);
	void *tags;
	XA_STATE(xas, &zimt_swap_pages, entry.val);

	xa_lock(&zimt_swap_pages);
	xas_for_each(&xas, tags, last_entry.val - 1) {
		__xa_erase(&zimt_swap_pages, xas.xa_index);
		zimt_free_tag_storage(tags);
	}
	xa_unlock(&zimt_swap_pages);
}

int arch_prepare_to_swap(struct folio *folio)
{
	long i, nr;
	int err;

	if (!zimt_hw_available())
		return 0;

	nr = folio_nr_pages(folio);
	for (i = 0; i < nr; i++) {
		err = zimt_save_tags_to_swap(page_swap_entry(folio_page(folio, i)),
					     folio_page(folio, i));
		if (err)
			goto out;
	}

	return 0;
out:
	while (i--)
		zimt_invalidate_swap_tags(swp_type(page_swap_entry(folio_page(folio, i))),
					  swp_offset(page_swap_entry(folio_page(folio, i))));
	return err;
}

void arch_swap_restore(swp_entry_t entry, struct folio *folio)
{
	long i, nr;

	if (!zimt_hw_available())
		return;

	nr = folio_nr_pages(folio);
	for (i = 0; i < nr; i++) {
		zimt_restore_tags_from_swap(entry, folio_page(folio, i));
		entry.val++;
	}
}
