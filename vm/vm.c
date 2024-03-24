/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include <hash.h>
#include "lib/kernel/hash.h"
#include "threads/mmu.h"
#include "vm/uninit.h"


/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */
void
vm_init (void) {
	vm_anon_init ();
	vm_file_init ();
#ifdef EFILESYS  /* For project 4 */
	pagecache_init ();
#endif
	register_inspect_intr ();
	/* DO NOT MODIFY UPPER LINES. */
	/* TODO: Your code goes here. */
}

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. */
enum vm_type
page_get_type (struct page *page) {
	int ty = VM_TYPE (page->operations->type);
	switch (ty) {
		case VM_UNINIT:
			return VM_TYPE (page->uninit.type);
		default:
			return ty;
	}
}

/* Helpers */
static struct frame *vm_get_victim (void);
static bool vm_do_claim_page (struct page *page);
static struct frame *vm_evict_frame (void);

/* Create the pending page object with initializer. If you want to create a
 * page, do not create it directly and make it through this function or
 * `vm_alloc_page`. */
bool
vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable,
		vm_initializer *init, void *aux) {
	bool ret;
	struct page *page  = NULL;

	ASSERT (VM_TYPE(type) != VM_UNINIT)
	struct thread *cur = thread_current();

	struct supplemental_page_table *spt = &cur->spt;

	/* Check wheter the upage is already occupied or not. */
	if (spt_find_page (spt, upage) == NULL) {
		/* TODO: Create the page, fetch the initialier according to the VM type,
		 * TODO: and then create "uninit" page struct by calling uninit_new. You
		 * TODO: should modify the field after calling the uninit_new. */


		page = (struct page *)malloc(sizeof(struct page));
		if(page == NULL){
			return false;
		}

		if(type == VM_ANON){
			uninit_new(page, upage, init, type,aux,anon_initializer); 
		}
		else if(type == VM_FILE){
			uninit_new(page, upage, init, type,aux,file_backed_initializer); 				
		}
		page->writable = writable;
		/* TODO: Insert the page into the spt. */
		if (!spt_insert_page(spt, page)) {
			return false;
		}
		return true;
	}
err:
	return false;
}

/* Find VA from spt and return page. On error, return NULL. */
struct page *
spt_find_page (struct supplemental_page_table *spt UNUSED, void *va UNUSED) {
	struct page *page = NULL;
	/* TODO: Fill this function. */
	printf(" spt find page va : %p\n", va);
	page = (struct page*)malloc(sizeof(struct page));
	struct hash_elem *e;

	page->va = pg_round_down(va);
	printf(" spt find page page->va : %p\n", page->va);
	printf(" spt find page spt : %p\n", spt);
	printf(" spt find page spt supli pt: %p\n", spt->supli_pt);
	printf(" spt find page page hash elem: %p\n", page->hash_elem);

	e = hash_find(&spt->supli_pt,&page->hash_elem);

	free(page);

	if(e != NULL){
		page =hash_entry(e, struct page, hash_elem);
	}
	return page;

	// struct hash supli_pt = spt->supli_pt;

	// struct hash_iterator i;
	// hash_first(&i, &supli_pt);

	// while(hash_next(&i)){
		
	// 	struct page *p = hash_entry(hash_cur(&i), struct page, hash_elem);
	// 	if (p->va == va){
	// 		return p;
	// 	}	
	// }
	// return page;
}

/* Insert PAGE into spt with validation. */
bool
spt_insert_page (struct supplemental_page_table *spt UNUSED,
		struct page *page UNUSED) {
	int succ = false;
	/* TODO: Fill this function. */
	
	if(hash_insert(&spt->supli_pt, &page->hash_elem) != NULL){
		succ = true;
	}

	return succ;
}

void
spt_remove_page (struct supplemental_page_table *spt, struct page *page) {
	vm_dealloc_page (page);
	return true;
}

/* Get the struct frame, that will be evicted. */
static struct frame *
vm_get_victim (void) {
	struct frame *victim = NULL;
	 /* TODO: The policy for eviction is up to you. */

	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame (void) {
	struct frame *victim UNUSED = vm_get_victim ();
	/* TODO: swap out the victim and return the evicted frame. */

	return NULL;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
static struct frame *
vm_get_frame (void) {
	struct frame *frame = NULL;
	/* TODO: Fill this function. */

	ASSERT (frame != NULL);
	ASSERT (frame->page == NULL);

	void *kva = palloc_get_page(PAL_USER); // USER 선언 안 하면 커널에서 가져오는것.
	if(kva == NULL){
		PANIC("todo");
	}

	frame = malloc(sizeof(struct frame));
	frame->kva = kva;
	// palloc() 및 프레임 가져오기.
	// 사용 가능 페이지가 없으면 희생자 페이지를 제거하고 반환. <- 이 아래는 Evict 함수로 구현하면 됨.(아마도.)
	// 항상 유효한 주소를 반환할 것.
	// 즉, 사용자 메모리가 가득 차면 프레임을 제거하여 사용 가능한 메모리 공간 확보.

	return frame;
}

/* Growing the stack. */
static void
vm_stack_growth (void *addr UNUSED) {
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp (struct page *page UNUSED) {
}

/* Return true on success */
bool
vm_try_handle_fault (struct intr_frame *f UNUSED, void *addr UNUSED,
		bool user UNUSED, bool write UNUSED, bool not_present UNUSED) {
	struct supplemental_page_table *spt UNUSED = &thread_current ()->spt;
	struct page *page = NULL;

	if(addr == NULL){
		return false;
	}

	if(is_kernel_vaddr(addr)){
		return false;
	}

	if(not_present){
		page = spt_find_page(&spt, addr);
		if(page == NULL){
			return false;
		}
		if(write == true && page->writable == false){
			return false;
		}
		return vm_do_claim_page (page);
	}
	// page자체가 말이 되는지 안되는지 확인하는법...
	// 그냥 null일때가 아니라 아예 kernel등이 아닌지 확인.


	/* TODO: Validate the fault */
	/* TODO: Your code goes here */

	return false;
}

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void
vm_dealloc_page (struct page *page) {
	destroy (page);
	free (page);
}

/* Claim the page that allocate on VA. */
bool
vm_claim_page (void *va UNUSED) {
	struct page *page = NULL;
	/* TODO: Fill this function */
	// va를 할당할 페이지를 요청.
	// 먼저 페이지를 가져온다음, 페이지로 do_claim page 호출.
	struct thread *cur = thread_current();

	page = spt_find_page(&cur->spt, va);
	if(page == NULL){
		return false;
	}
	return vm_do_claim_page (page);
}

/* Claim the PAGE and set up the mmu. */
static bool
vm_do_claim_page (struct page *page) {
	struct frame *frame = vm_get_frame ();

	/* Set links */
	frame->page = page;
	page->frame = frame;

	/* TODO: Insert page table entry to map page's VA to frame's PA. */
		// claim page = 물리적 프레임, 페이지 할당.
	// vm_get_frame으로 프레임을 얻고
	// 페이지 테이블의 가상 주소 - 실제 주소 매핑 추가.
	// 반환값은 작업 성공 여부.

	struct thread *cur = thread_current();
	pml4_set_page(cur->pml4, page->va, frame->kva, page->writable);

	return swap_in (page, frame->kva);
}

/* Initialize new supplemental page table */
void
supplemental_page_table_init (struct supplemental_page_table *spt UNUSED) {

	hash_init(&spt->supli_pt, page_hash, page_less, NULL);

}

/* Copy supplemental page table from src to dst */
bool
supplemental_page_table_copy (struct supplemental_page_table *dst UNUSED,
		struct supplemental_page_table *src UNUSED) {
}

/* Free the resource hold by the supplemental page table */
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
}

unsigned 
page_hash (const struct hash_elem *p_, void *aux UNUSED){

	const struct page *p = hash_entry (p_, struct page, hash_elem);
	return hash_bytes (&p->va, sizeof p->va);
}

bool 
page_less (const struct hash_elem *a_, const struct hash_elem *b_, void *aux UNUSED){
 	const struct page *a = hash_entry (a_, struct page, hash_elem);
  const struct page *b = hash_entry (b_, struct page, hash_elem);

  	return a->va < b->va;
}

struct page *
page_lookup (const void *address){
	 struct page p;
  	struct hash_elem *e;
	struct supplemental_page_table supli_p = thread_current()->spt;
  	p.va = address;
  	e = hash_find (&supli_p.supli_pt, &p.hash_elem);
  	return e != NULL ? hash_entry (e, struct page, hash_elem) : NULL;
}