/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"
#include "userprog/process.h"
#include "threads/vaddr.h"
#include "string.h"
#include "threads/mmu.h"

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
	struct page_load_data *aux = (struct page_load_data *) page->uninit.aux;
	file_page->file = aux->file;
	file_page->ofs = aux->ofs;
	file_page->read_bytes = aux->read_bytes;
	file_page->zero_bytes = aux->zero_bytes;
}

/* Swap in the page by read contents from the file. */
static bool
file_backed_swap_in (struct page *page, void *kva) {
	struct file_page *file_page UNUSED = &page->file;
	//printf("file backed swap in!\n");

	// void *va  = do_mmap(kva, file_length(page->file.file), page->writable, page->file.file, page->file.ofs);
	// if(va == kva){
	// 	return true;
	// }

	// return false;
	//page->frame->kva = kva;
	struct page_load_data *aux_d = (struct page_load_data *)(page->uninit.aux);

	file_seek(aux_d->file, aux_d->ofs);

	if(file_read(aux_d->file, kva, aux_d->read_bytes) != (int)aux_d->read_bytes){
		palloc_free_page(page->frame->kva);
	return false;
	}
	memset((page->frame->kva)+(aux_d->read_bytes), 0, aux_d->zero_bytes);
	// struct swap_table_entry *swe = page->swe;

	// swe->is_empty = false;
	// swe->owner = NULL;

	return true;

}

/* Swap out the page by writeback contents to the file. */
static bool
file_backed_swap_out (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
	
	// do_munmap(pg_round_down(page->va));

	// return true;

	// struct list_elem *e;
	// if(!list_empty(&mmap_list)){
	// 	for(e = list_begin(&mmap_list);e != list_end(&mmap_list);e = list_next(e)){
	// 		struct page *p = list_entry(e, struct page, f_elem);
			
	// 	}
	// }

	if(page == NULL){
		return false;
	}

	// int count = page->mapping_count;
	// void * addr = pg_round_down(page->va);

	

	// for(int i = 0; i <count; i++){
	// 	if(page){
	// 		file_backed_destroy(page);
	// 		addr+= PGSIZE;
	// 		page = spt_find_page(&thread_current()->spt, addr);

	// 	}
	// }

	// struct swap_table_entry *swe = page->swe;

	// swe->is_empty = true;
	// swe->owner = page;
	

	// return true;
	struct page_load_data *aux_d = (struct page_load_data *)page->uninit.aux;

	if(pml4_is_dirty(thread_current()->pml4, page->va)){
		file_write_at(aux_d->file, page->va, aux_d->read_bytes, aux_d->ofs);
		pml4_set_dirty(thread_current()->pml4, page->va, 0);
	}

	pml4_clear_page(thread_current()->pml4, page->va);

}

/* Destory the file backed page. PAGE will be freed by the caller. */
static void
file_backed_destroy (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;

	if(pml4_is_dirty(thread_current()->pml4, page->va)){
		file_write_at(file_page->file, page->va, file_page->read_bytes, file_page->ofs);
		pml4_set_dirty(thread_current()->pml4, page->va, 0);
	}
	pml4_clear_page(thread_current()->pml4, page->va);
}

/* Do the mmap */
void *
do_mmap (void *addr, size_t length, int writable,
		struct file *file, off_t offset) {
		
	bool succ = false;
	struct file * f = file_reopen(file);
	void * new_addr = addr;
	int count = length <= PGSIZE ? 1 : (length%PGSIZE ? length/PGSIZE +1: length/PGSIZE);
	// printf("do mmap \n");
	// printf("do mmap length : %d\n", length);
	// printf("do mmap addr : %p\n", addr);
	
	size_t read_bytes = file_length(f) < length ? file_length(f) : length;
	size_t zero_bytes = PGSIZE - (read_bytes%PGSIZE) ;
	while(read_bytes > 0 || zero_bytes > 0){
		size_t page_read_bytes =  read_bytes <PGSIZE ? read_bytes: PGSIZE;
		size_t page_zero_bytes = PGSIZE - page_read_bytes;
		struct page_load_data *aux_d = malloc(sizeof(struct page_load_data));
		aux_d->file = f;
//		//printf("do mmap file reopen file : %p\n", aux_d->file);
		aux_d->ofs = offset;
		aux_d->read_bytes = page_read_bytes;
		aux_d->zero_bytes = page_zero_bytes;
	//	//printf("do mmap aux_d ofs:%d\n",offset);
		//printf("do mmap aux_d  read bytes:%d\n",page_read_bytes);
		//printf("do mmap aux_d zero bytes : %d\n", page_zero_bytes);
		if(!vm_alloc_page_with_initializer(VM_FILE, new_addr, writable, lazy_load_segment, aux_d)){
			//printf("do mmap vm alloc page with initializer fail!\n");
			return NULL;
		}

		read_bytes -= page_read_bytes;
		zero_bytes -= page_zero_bytes;
		new_addr += PGSIZE;
		offset += page_read_bytes;

	}
	struct page *p = spt_find_page(&thread_current()->spt, addr);
	p->mapping_count = count;
	if(read_bytes== 0){
		//printf(" do mmap length == 0!\n");
		succ = true;
	}
	if(succ){
		//printf("do mmap success!\n");
		//printf("do mmap final addr : %p\n", addr);
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
	//printf("do munmap spt find 직후\n");
	if(p == NULL){
		//printf("do munmap p == NULL\n");
		exit(-1);
	}
	int count = p->mapping_count;
	//printf("do munmap count : %d\n", count);

	for(int i =0; i < count;i++){
		//printf("do munmap while count 도는 중...\n");
		if(p){
			destroy(p);
			addr += PGSIZE;
			p = spt_find_page(&thread_current()->spt, addr);
			//printf("do munmap find p!\n");
		}
		//printf("do munmap if문 박!\n");
	}
	//printf("do munmap!\n");
	// if(VM_TYPE(p->operations->type) == VM_FILE){
	// 	if(p->modified){
	// 		file_seek(p->origin, p->ofs);
	// 		if(file_write(p->origin, p->frame->kva,p->read_bytes) != (int)p->read_bytes){
	// 			exit(-1);
	// 		}
	// 		vm_dealloc_page(p);
	// 	}
	// 	else{
	// 		vm_dealloc_page(p);
	// 	}
	// }

}



bool
lazy_load (struct page *page, void *aux){
	struct page_load_data *aux_d = (struct page_load_data *)aux;
	struct file *file;
	//printf("file.c lazy load 진입\n");
	uint32_t page_read_bytes = aux_d->read_bytes;
	uint32_t page_zero_bytes = aux_d->zero_bytes;
	file = aux_d->file;
	//printf("file.c lazy load aux_d read bytes: %d\n",page_read_bytes);
	//printf("file.c lazy load aux_d zero bytes: %d\n",page_zero_bytes);
	//printf("file.c lazy load aux_d file: %p\n",file);
	//printf("file.c lazy load aux_d ofs: %d\n",aux_d->ofs);
	file_seek(file, aux_d->ofs);
	// printf("file.c lazy load file read bytes : %d\n", file_read(file, page->frame->kva, page_read_bytes));
	// printf("file.c lazy load file size :%d\n", file_length(file));
	uint32_t length = file_length(file);
	if(file_read(file, page->frame->kva, page_read_bytes) != (int)length){
		//printf("file.c lazy loac file read fail!\n");
		palloc_free_page(page->frame->kva);
		return false;
	}
	if(length < page_read_bytes){
		page_zero_bytes = PGSIZE - length;
	}
	memset((page->frame->kva) +(length), 0, page_zero_bytes);
	page->read_bytes = length;
	page->ofs = aux_d->ofs;
	return true;

}