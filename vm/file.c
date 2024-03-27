/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"
#include "userprog/process.h"
#include "threads/vaddr.h"
#include "string.h"

static bool file_backed_swap_in (struct page *page, void *kva);
static bool file_backed_swap_out (struct page *page);
static void file_backed_destroy (struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations file_ops = {
	.swap_in = file_backed_swap_in,
	.swap_out = file_backed_swap_out,
	.destroy = file_backed_destroy,
	.type = VM_FILE,
};

/* The initializer of file vm */
void
vm_file_init (void) {
}

/* Initialize the file backed page */
bool
file_backed_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	page->operations = &file_ops;

	struct file_page *file_page = &page->file;
}

/* Swap in the page by read contents from the file. */
static bool
file_backed_swap_in (struct page *page, void *kva) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Swap out the page by writeback contents to the file. */
static bool
file_backed_swap_out (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Destory the file backed page. PAGE will be freed by the caller. */
static void
file_backed_destroy (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Do the mmap */
void *
do_mmap (void *addr, size_t length, int writable,
		struct file *file, off_t offset) {

	bool succ = false;
	while(length >0){
		size_t page_read_bytes = length <PGSIZE ? length : PGSIZE;
		size_t page_zero_bytes = PGSIZE - page_read_bytes;
		struct page_load_data *aux_d = malloc(sizeof(struct page_load_data));
		aux_d->file = file_reopen(file);
		aux_d->ofs = offset;
		aux_d->read_bytes = page_read_bytes;
		aux_d->zero_bytes = page_zero_bytes;

		if(!vm_alloc_page_with_initializer(VM_FILE, addr, writable, lazy_load, aux_d)){
			return NULL;
		}
		length -= page_read_bytes;
		addr += PGSIZE;
		offset += page_read_bytes;
	}
	if(length == 0){
		succ = true;
	}
	if(succ){
		return addr;
	}
	else{
		return NULL;
	}
	

}

/* Do the munmap */
void
do_munmap (void *addr) {

	struct page *p;
	p = spt_find_page(&thread_current()->spt, addr);
	struct file *f;

	if(p == NULL){
		printf("do munmap p == NULL\n");
		exit(-1);
	}

	if(VM_TYPE(p->operations->type) == VM_FILE){
		if(p->modified){
			file_seek(p->origin, p->ofs);
			if(file_write(p->origin, p->frame->kva,p->read_bytes) != (int)p->read_bytes){
				exit(-1);
			}
			vm_dealloc_page(p);
		}
		else{
			vm_dealloc_page(p);
		}
	}

}



bool
lazy_load (struct page *page, void *aux){
	struct page_load_data *aux_d = (struct page_load_data *)aux;
	struct file *file;
	uint32_t page_read_bytes = aux_d->read_bytes;
	uint32_t page_zero_bytes = aux_d->zero_bytes;
	file = aux_d->file;
	file_seek(file, aux_d->ofs);
	if(file_read(file, page->frame->kva, page_read_bytes) != (int)page_read_bytes){
		palloc_free_page(page->frame->kva);
		return false;
	}
	memset((page->frame->kva) +(page_read_bytes), 0, page_zero_bytes);
	page->origin = file;
	page->read_bytes = page_read_bytes;
	page->ofs = aux_d->ofs;
	return true;

}