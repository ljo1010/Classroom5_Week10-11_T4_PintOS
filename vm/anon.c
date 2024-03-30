/* anon.c: Implementation of page for non-disk image (a.k.a. anonymous page). */

#include "vm/vm.h"
#include "devices/disk.h"
#include "kernel/bitmap.h"
#include "string.h"
#include "vm/vm.h"
#include "userprog/process.h"

static struct bitmap *swap_bit;

/* DO NOT MODIFY BELOW LINE */
static struct disk *swap_disk;
static bool anon_swap_in (struct page *page, void *kva);
static bool anon_swap_out (struct page *page);
static void anon_destroy (struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations anon_ops = {
	.swap_in = anon_swap_in,
	.swap_out = anon_swap_out,
	.destroy = anon_destroy,
	.type = VM_ANON,
};

/* Initialize the data for anonymous pages */
void
vm_anon_init (void) {
	/* TODO: Set up the swap_disk. */
    swap_disk = disk_get(1, 1);
    swap_bit = bitmap_create((size_t)disk_size(swap_disk));
	// bitmap_set_all(swap_bit,true);

}

/* Initialize the file mapping */
bool
anon_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	struct uninit_page *uninit = &page->uninit;
	memset(uninit, 0, sizeof(struct uninit_page));

	page->operations = &anon_ops;

	struct anon_page *anon_page = &page->anon;
	anon_page->swap_index = UINT32_MAX;
	anon_page->thread = thread_current();

	return true;
}

/* Swap in the page by read contents from the swap disk. */
static bool
anon_swap_in (struct page *page, void *kva) {
	//printf("anon swap in 진입!\n");
	  struct anon_page *anon_page = &page->anon;

    if (anon_page->swap_index == SIZE_MAX)
        return false;

 
    bool check = bitmap_contains(swap_bit, anon_page->swap_index, 8, false);
    if (check)
    {
        return false;
    }

    for (int i = 0; i < 8; i++)
    {
        disk_read(swap_disk, anon_page->swap_index + i, kva + i * DISK_SECTOR_SIZE);
    }

    bitmap_set_multiple(swap_bit, anon_page->swap_index, 8, false);


    return true;

}

/* Swap out the page by writing contents to the swap disk. */
static bool
anon_swap_out (struct page *page) {
	//printf("anon swap out 진입!\n");
	 struct anon_page *anon_page = &page->anon;
 
    disk_sector_t sec_no = (disk_sector_t)bitmap_scan_and_flip(swap_bit, 0, 8, false);

    if (sec_no == BITMAP_ERROR)
        return false;

    anon_page->swap_index = sec_no;

    for (int i = 0; i < 8; i++)
    {
        disk_write(swap_disk, sec_no + i, page->frame->kva + i * DISK_SECTOR_SIZE);
    }

    pml4_clear_page(anon_page->thread->pml4, page->va);
    pml4_set_dirty(anon_page->thread->pml4, page->va, false);
    page->frame = NULL;
	//printf("anon swap out 끝!!\n");
    return true;

}

/* Destroy the anonymous page. PAGE will be freed by the caller. */
static void
anon_destroy (struct page *page) {
	struct anon_page *anon_page = &page->anon;
	// printf("anon destroy page :%p\n", page);

	// printf("anon destroy page uninit aux :%p\n", page->uninit.aux);
	// printf("anon destroy page uninit :%p\n", page->uninit);
	
	// free((struct page_load_data*)page->uninit.aux);
    if (page->frame != NULL)
    {
        // printf("anon_destroy: %s\n", thread_current()->name);
        // printf("remove: %p, kva:%p\n", page->va, page->frame->kva);
        // printf("list_size: %d, list: %p\n", list_size(&lru), &lru);


        list_remove(&page->frame->frame_elem);
 

        // printf("anon_destroy: list: %p\n", &lru);

        // pte write bit 1 -> free
        free(page->frame);
    }
    if (anon_page->swap_index != SIZE_MAX)
        bitmap_set_multiple(swap_bit, anon_page->swap_index, 8, false);
}
