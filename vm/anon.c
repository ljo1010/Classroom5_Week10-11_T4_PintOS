/* anon.c: Implementation of page for non-disk image (a.k.a. anonymous page). */

#include "vm/vm.h"
#include "devices/disk.h"
#include "kernel/bitmap.h"
#include "string.h"
#include "vm/vm.h"
#include "userprog/process.h"

static struct bitmap *swap_bit;
static struct lock bitmap_lock;

extern struct list swap;
extern struct lock swap_lock;


struct bitmap *swap_map;

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
    // swap_disk = disk_get(1, 1);
    // swap_bit = bitmap_create((size_t)disk_size(swap_disk));
	// lock_init(&bitmap_lock);

    	swap_disk = disk_get(1,1); // swap disk 
	size_t swap_size = disk_size(swap_disk) / SECTORS_PER_PAGE;
	swap_map = bitmap_create(swap_size);
	lock_init(&swap_lock);

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
    //printf("anon swap in page: %p\n", page);
	//   struct anon_page *anon_page = &page->anon;

    // if (anon_page->swap_index == SIZE_MAX)
    //     return false;

	// lock_acquire(&bitmap_lock);
    // bool check = bitmap_contains(swap_bit, anon_page->swap_index, 8, false);
	// lock_release(&bitmap_lock);
    // if (check)
    // {
    //     return false;
    // }

    // for (int i = 0; i < 8; i++)
    // {
    //     disk_read(swap_disk, anon_page->swap_index + i, kva + i * DISK_SECTOR_SIZE);
    // }
	// lock_acquire(&bitmap_lock);
    // bitmap_set_multiple(swap_bit, anon_page->swap_index, 8, false);
	// lock_release(&bitmap_lock);
	// //printf("anon swap in!\n");

    // return true;

    	// printf("[START] anon_swap_in {%p}\n",page->va);
	struct anon_page *anon_page = &page->anon;
	
	int page_no = anon_page->swap_idx;

	// 유효한 swap map 인지 확인
	if(bitmap_test(swap_map,page_no) == false){
		return false;
	}

	// 한 페이지의 sector의 개수만큼 sector에서 read
	for (int i = 0 ; i < SECTORS_PER_PAGE ; i ++)
	{
		lock_acquire(&swap_lock);
		disk_read(swap_disk, page_no * SECTORS_PER_PAGE + i , page->va + DISK_SECTOR_SIZE * i );
		lock_release(&swap_lock);
	}

	// 사용 가능한 swap map으로 변경
	bitmap_set(swap_map,page_no,false);
	// printf("[END] anon_swap_in {%p}\n",page->va);
	return true;

}

/* Swap out the page by writing contents to the swap disk. */
static bool
anon_swap_out (struct page *page) {

    // printf("[START] anon_swap_out {%p}\n",page->va);
	struct anon_page *anon_page = &page->anon;

	// 빈 swap slot 찾기
	int page_no = bitmap_scan(swap_map,0,1,false);

	// 한 페이지의 sector의 개수만큼 sector에 write
	for (int i = 0 ; i < SECTORS_PER_PAGE ; i ++)
	{
		lock_acquire(&swap_lock);
		disk_write(swap_disk, page_no * SECTORS_PER_PAGE + i , page->va + DISK_SECTOR_SIZE * i );
		lock_release(&swap_lock);
	}

	// swap_map 셋팅
	bitmap_set(swap_map, page_no, true);

	//clear page
	pml4_clear_page(thread_current()->pml4,page->va);

	anon_page->swap_idx = page_no;
	// printf("[END] anon_swap_out {%p}\n",page->va);
	return true;

	// //printf("anon swap out 진입!\n");
	//  struct anon_page *anon_page = &page->anon;
	// lock_acquire(&bitmap_lock);
    // disk_sector_t sec_no = (disk_sector_t)bitmap_scan_and_flip(swap_bit, 0, 8, false);
	// lock_release(&bitmap_lock);
    // if (sec_no == BITMAP_ERROR)
    //     return false;

    // anon_page->swap_index = sec_no;

    // for (int i = 0; i < 8; i++)
    // {
    //     disk_write(swap_disk, sec_no + i, page->frame->kva + i * DISK_SECTOR_SIZE);
    // }

    // pml4_clear_page(anon_page->thread->pml4, page->va);
    // pml4_set_dirty(anon_page->thread->pml4, page->va, false);
    // page->frame = NULL;
	// //printf("anon swap out 끝!!\n");
    // return true;

}

/* Destroy the anonymous page. PAGE will be freed by the caller. */
static void
anon_destroy (struct page *page) {
	struct anon_page *anon_page = &page->anon;
	// printf("anon destroy page :%p\n", page);

	// printf("anon destroy page uninit aux :%p\n", page->uninit.aux);
	// printf("anon destroy page uninit :%p\n", page->uninit);
	
	// free((struct page_load_data*)page->uninit.aux);
    // if (page->frame != NULL)
    // {
    //     // printf("anon_destroy: %s\n", thread_current()->name);
    //     // printf("remove: %p, kva:%p\n", page->va, page->frame->kva);
    //     // printf("list_size: %d, list: %p\n", list_size(&lru), &lru);

	// 	lock_acquire(&swap_lock);
    //     list_remove(&page->frame->frame_elem);
	// 	lock_release(&swap_lock);

    //     // printf("anon_destroy: list: %p\n", &lru);

    //     // pte write bit 1 -> free
    //     free(page->frame);
    // }
    // if (anon_page->swap_index != SIZE_MAX)
    //     bitmap_set_multiple(swap_bit, anon_page->swap_index, 8, false);
}
