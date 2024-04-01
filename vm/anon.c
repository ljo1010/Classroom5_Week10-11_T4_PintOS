/* anon.c: Implementation of page for non-disk image (a.k.a. anonymous page). */
//익명성 페이지
#include "vm/vm.h"
#include "devices/disk.h"

/* DO NOT MODIFY BELOW LINE */
static struct disk *swap_disk;
static bool anon_swap_in (struct page *page, void *kva);
static bool anon_swap_out (struct page *page);
static void anon_destroy (struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations anon_ops =
{
	.swap_in = anon_swap_in,
	.swap_out = anon_swap_out,
	.destroy = anon_destroy,
	.type = VM_ANON,
};

/* Initialize the data for anonymous pages */
void
vm_anon_init (void)
{
	/* TODO: Set up the swap_dis k. */
	swap_disk = disk_get(1, 1);
	list_init(&swap_table);
	lock_init(&swap_table_lock);

	/*디스크 크기정도의 slot을 만들어서 table에 넣기
	  1sector = 512 바이트, 1 page = 4096bytes(PGSIZE) -> 1 slot = 8 sector*/
	disk_sector_t swap_size = disk_size(swap_disk) / 8;
	for (disk_sector_t i = 0; i < swap_size; i++)
	{
		struct slot *slot = (struct slot *)malloc(sizeof(struct slot));
		slot->page = NULL;
		slot->slot_number = i;
		lock_acquire(&swap_table_lock);
		list_push_back(&swap_table, &slot->swap_elem);
		lock_release(&swap_table_lock);
	}

	 
}

/* Initialize the file mapping */
bool
anon_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	page->operations = &anon_ops;

	struct anon_page *anon_page = &page->anon;
	anon_page->slot_number = -1;
	return true;
}

/* Swap in the page by read contents from the swap disk. */
static bool
anon_swap_in (struct page *page, void *kva) 
{
	struct anon_page *anon_page = &page->anon;
	disk_sector_t page_slot_no = anon_page->slot_number;
	struct list_elem *e;
	struct slot *slot;
	lock_acquire(&swap_table_lock);
	
	for(e = list_begin(&swap_disk); e != list_end(&swap_table); e = list_next(e))
	{	
		slot = list_entry(e, struct slot, swap_elem);
		if(slot->slot_number == page_slot_no)
		{
		for(int i = 0; i < 8; i++)
		{
			disk_read(swap_disk, page_slot_no*8 + i, kva + DISK_SECTOR_SIZE*i);
		}
		slot->page = NULL;
		anon_page->slot_number = -1;
		lock_release(&swap_table_lock);
		return true;

		}
	}
	lock_release(&swap_table_lock);
	return false;

}

/* Swap out the page by writing contents to the swap disk. */
static bool
anon_swap_out (struct page *page) 
{
	if(page == NULL)
	{
		return false;
	}
	struct anon_page *anon_page = &page->anon;
	struct list_elem *e;
	struct slot *slot;

	lock_acquire(&swap_table_lock);
	


	for(e = list_begin(&swap_disk); e != list_end(&swap_table); e = list_next(e))
	{
		slot = list_entry(e, struct slot, swap_elem);
		if(slot->page == NULL)
		{
			for(int i = 0; i < 8; i++)
			{
				disk_write(swap_disk, slot->slot_number*8 + i, page->va + DISK_SECTOR_SIZE*i);
			}
			anon_page->slot_number = slot->slot_number;
			slot->page = page;
			page->frame->page = NULL;
			page->frame = NULL;
			pml4_clear_page(thread_current()->pml4, page->va);
			lock_release(&swap_table_lock);
			return true;
		}
	}
	lock_release(&swap_table_lock);
	PANIC("there`s no swap page");
}

/* Destroy the anonymous page. PAGE will be freed by the caller. */
static void
anon_destroy (struct page *page) 
{
	struct anon_page *anon_page = &page->anon;

	struct list_elem *elem;
	struct slot * slot;

	lock_acquire(&swap_table_lock);
	for (elem = list_begin(&swap_table); elem != list_end(&swap_table); elem = list_next(elem))
	{
		slot = list_entry(elem, struct slot, swap_elem);
		if(slot->slot_number == anon_page->slot_number)
		{
			slot->page = NULL;
			break;
		}
	}	
	lock_release(&swap_table_lock);
}
