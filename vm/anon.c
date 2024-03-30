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
	swap_disk = NULL;
	swap_disk = disk_get(1,1);
	uint32_t size = disk_size(swap_disk)/ 8;
	//printf("vm anon init size : %u\n", size);
	swap_bit = bitmap_create(size);
	// bitmap_set_all(swap_bit,true);

}

/* Initialize the file mapping */
bool
anon_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	page->operations = &anon_ops;

	struct anon_page *anon_page = &page->anon;
}

/* Swap in the page by read contents from the swap disk. */
static bool
anon_swap_in (struct page *page, void *kva) {
	//printf("anon swap in 진입!\n");
	struct anon_page *anon_page = &page->anon;
	struct swap_table_entry *swe = page->swe;
	uint32_t sector_start = swe->sec_idx_start;

	page->frame->kva = kva;

	for(int i = 0 ; i <8 ; ++i){
		disk_read(swap_disk, (sector_start*8)+i, page->frame->kva+(i*512));
	}

	bitmap_set(swap_bit, swe->sec_idx_start,false);

	swe->is_empty = false;
	swe->owner = NULL;
	swe->sec_idx_start = -1;
	return true;

}

/* Swap out the page by writing contents to the swap disk. */
static bool
anon_swap_out (struct page *page) {
	struct anon_page *anon_page = &page->anon;
	printf("anon swap out 진입!\n");
	size_t index = bitmap_scan_and_flip(swap_bit,0, 1,false);
	if(index == BITMAP_ERROR){
		return false;
	}
	struct swap_table_entry *swe = page->swe;
	printf("anon swap out sec_idx_start : %d\n", index);
	swe->sec_idx_start = index;
	printf("anon swap out page : %p\n", page);
	printf("anon swap out swap disk : %p\n", swap_disk);

	for(int i = 0 ; i <8 ; i++){
		disk_write(swap_disk, (index*8)+i, page->frame->kva +(i * 512));
		printf("anon swap out page frame kva+(i*512) : %p\n",  page->frame->kva +(i * 512));
		
	}
	pml4_clear_page(thread_current()->pml4, page->va);
	swe->is_empty = true;

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
}
