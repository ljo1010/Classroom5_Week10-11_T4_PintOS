/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include <hash.h>
#include "lib/kernel/hash.h"
#include "threads/mmu.h"
#include "vm/uninit.h"
#include "string.h"


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
	struct page *page;

	ASSERT (VM_TYPE(type) != VM_UNINIT)
	struct thread *cur = thread_current();

	struct hash *spt = &cur->spt;
	////printf("vm alloc page with initializer 진입\n");
	/* Check wheter the upage is already occupied or not. */
	if (spt_find_page (spt, upage) == NULL) {
		/* TODO: Create the page, fetch the initialier according to the VM type,
		 * TODO: and then create "uninit" page struct by calling uninit_new. You
		 * TODO: should modify the field after calling the uninit_new. */
		////printf("vm alloc page with initializer spt find page == NULL\n");

		page = (struct page *)malloc(sizeof(struct page));
		if(page == NULL){
			////printf("vm alloc with initializer page == NULL\n");
			return false;
		}
		switch (VM_TYPE(type))
		{
		case VM_ANON:
			////printf("vm alloc with initializer page type : VM_ANON\n");
			uninit_new(page, upage, init, type,aux,anon_initializer); 
			////printf("vm alloc with initializer page type : VM_ANON : uninit_new \n");
			break;
		case VM_FILE:
			////printf("vm alloc with initializer page type : VM_FILE\n");
			uninit_new(page, upage, init, type,aux,file_backed_initializer); 	
		default:
			break;
		}
		page->writable = writable;
		////printf("vm alloc with initializer writable 설정\n");
		/* TODO: Insert the page into the spt. */
		if (!spt_insert_page(spt, page)) {
			////printf("vm alloc with initializer insert 실패\n");
			return false;
		}
		return true;
	}
	////printf("vm alloc page with initializer spt find not null\n");
err:
	return false;
}

/* Find VA from spt and return page. On error, return NULL. */
struct page *
spt_find_page (struct hash *spt, void *va) {
	struct page *page = NULL;
	/* TODO: Fill this function. */
	////printf(" spt find page va : %p\n", va);
	page = (struct page*)malloc(sizeof(struct page));
	struct hash_elem *e;

	page->va = pg_round_down(va);
	////printf(" spt find page page->va : %p\n", page->va);
	////printf(" spt find page spt : %p\n", spt);
	////printf(" spt find page page hash elem: %p\n", page->hash_elem);

	////printf("spt find page thread_current() : %s\n", thread_current()->name);

	e = hash_find(spt,&page->hash_elem);
	////printf(" spt find page e: %p\n",e);
	if(e == NULL){
		////printf("spt find page e == NULL\n");
		return NULL;
	}

	if(e != NULL){
		page =hash_entry(e, struct page, hash_elem);
		return page;
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
spt_insert_page (struct hash *spt,
		struct page *page) {
	int succ = false;
	/* TODO: Fill this function. */
	////printf("spt insert page spt : %p\n", spt);
	////printf("spt insert page page hash elem : %p\n", page->hash_elem);

	////printf("spt insert page thread_current() : %s\n", thread_current()->name);

	////printf("spt insert page page->va : %p\n", page->va);
	if(!hash_insert(spt, &page->hash_elem)){
		////printf("spt insert page hash insert succ\n");
		succ = true;
	}

	return succ;
}

void
spt_remove_page (struct hash *spt, struct page *page) {
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
	struct frame *frame;
	/* TODO: Fill this function. */

	void *kva = palloc_get_page(PAL_USER); // USER 선언 안 하면 커널에서 가져오는것.
	if(kva == NULL){
		PANIC("todo");
	}

	frame = malloc(sizeof(struct frame));
	frame->kva = kva;
	frame->page = NULL;

	////printf("vm get frame frame page : %p\n",frame->page);
	// palloc() 및 프레임 가져오기.
	// 사용 가능 페이지가 없으면 희생자 페이지를 제거하고 반환. <- 이 아래는 Evict 함수로 구현하면 됨.(아마도.)
	// 항상 유효한 주소를 반환할 것.
	// 즉, 사용자 메모리가 가득 차면 프레임을 제거하여 사용 가능한 메모리 공간 확보.

	ASSERT (frame != NULL);
	ASSERT (frame->page == NULL);

	return frame;
}

/* Growing the stack. */
static void
vm_stack_growth (void *addr UNUSED) {

	// 페이지식으로 정렬.
	addr = pg_round_down(addr);

	// 현 rsp 내에 addr이 있다면 이미 할당된거니 스루.

	// 내가 늘려야하는 stack의 사이즈를 구하고
	uint8_t size = addr - thread_current()->cur_rsp;
	void * new_addr = thread_current()->cur_rsp;

	// size 만큼 늘릴때까지 새 addr에 계속 할당.
	while(size != 0){
		int iter = size/PGSIZE;
		if(iter > 0){
			new_addr += PGSIZE;
		}
		else{
			new_addr += (size%PGSIZE);
		}
		if(vm_alloc_page(VM_ANON | VM_MARKER_0, new_addr, true)){
			if(vm_claim_page(new_addr)){
				// 현 rsp를 갱신해야하는데 어디서 ..?!?!?!?!?! 
				// 구조체에 어떻게 저장한다고 해도 현 진짜 intr_frame에 적용하는건 대체 어케하라는거..
				// 별개의 함수를 만들어서 try handle fault로 넘어오는 Intr_frame을 갱신하는 일이 의미가 있나. 
				thread_current()->tf.rsp += PGSIZE;
				thread_current()->cur_rsp += PGSIZE;
				size -= PGSIZE;
			}
		}
	}
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp (struct page *page UNUSED) {
}

/* Return true on success */
bool
vm_try_handle_fault (struct intr_frame *f UNUSED, void *addr UNUSED,
		bool user UNUSED, bool write UNUSED, bool not_present UNUSED) {
	struct hash *spt UNUSED = &thread_current ()->spt;
	struct page *page = NULL;
	////printf("vm try handle fault 진입\n");
	void *stack_bottom = (void *) (((uint8_t *) USER_STACK) - PGSIZE); // 0x4747F000 USER_STACK 0x47480000
	void *stack_max = (void *) (((uint8_t *) USER_STACK) - (1<<20)); //0x4757F000
	// //printf(" vm try handle fault addr : %p\n", addr);
	if(addr == NULL){
		return false;
	}
	if(is_kernel_vaddr(addr)){
		// //printf("vm try handl fault kernel vaddr!\n");
		return false;
	}
	printf("vm try handl fault thread current cur rsp : %p\n",thread_current()->cur_rsp);
	if(stack_max > addr > stack_bottom && thread_current()->cur_rsp < addr){
		vm_stack_growth(addr);
	}
	// struct page *exam;
	// exam = spt_find_page(spt, addr);
	// if((exam->operations->type)& VM_MARKER_0_MASK){
	// 	vm_stack_growth(addr);
	// }

	if(not_present){
		// //printf("vm try handl fault not present!\n");
		page = spt_find_page(spt, addr);
		if(page == NULL){
			// //printf("vm try handl fault not present and page == NULL!\n");
			return false;
		}
		if(write == true && page->writable == false){
			// //printf("vm try handl fault not present and write none!\n");
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
		////printf("vm claim page : page == NULL\n");
		return false;
	}
	return vm_do_claim_page (page);
}

/* Claim the PAGE and set up the mmu. */
static bool
vm_do_claim_page (struct page *page) {
	struct frame *frame = vm_get_frame ();
	////printf("vm do claim page\n");
	/* Set links */
	frame->page = page;
	page->frame = frame;
	////printf("#############1111##############\n");
	/* TODO: Insert page table entry to map page's VA to frame's PA. */
		// claim page = 물리적 프레임, 페이지 할당.
	// vm_get_frame으로 프레임을 얻고
	// 페이지 테이블의 가상 주소 - 실제 주소 매핑 추가.
	// 반환값은 작업 성공 여부.

	struct thread *cur = thread_current();
	if(!pml4_set_page(cur->pml4, page->va, frame->kva, page->writable)){
		//printf("vm do claim page pml4 set page fail!\n");
		return false;
	}
	////printf("##############2222#############\n");
	return swap_in (page, frame->kva);
}

/* Initialize new supplemental page table */
void
spt_hash_init (struct hash *spt UNUSED) {
	////printf("spt hash init\n");
	hash_init(spt, page_hash, page_less, NULL);

}

/* Copy supplemental page table from src to dst */
bool
spt_hash_copy (struct hash *dst UNUSED,
		struct hash *src UNUSED) {
	
	// //printf("spt hash copy 진입\n");
	struct hash_iterator i;
	hash_first(&i, src);
	while(hash_next(&i)){
		// //printf("spt hash copy while hash next 도는중...\n");
		struct page *p = hash_entry(hash_cur(&i), struct page, hash_elem);
		//printf("spt hash copy type :%p\n", p->operations->type);
		//printf("spt hash copy page p :%p\n", p);
		switch (VM_TYPE(p->operations->type))
		{
		case VM_UNINIT:
			//printf("spt hash copy VM_UNINIT\n");
			if(!vm_alloc_page_with_initializer(p->uninit.type, p->va, p->writable, p->uninit.init,p->uninit.aux)){
				return false;
			}
			break;
		default:
			//printf("spt hash copy default\n");
			if(vm_alloc_page(p->operations->type, p->va, p->writable)){
				if(!vm_claim_page(p->va)){
					return false;
				}
			}
			break;
		}
		if(p->operations->type != VM_UNINIT){
			struct page *k;
			//printf("spt hash copy dst : %p\n", dst);
			k = spt_find_page(dst, p->va);
			//printf("spt hash copy page k :%p\n");
			// if(k == NULL){
			// 	return false;
			// }
			//printf("spt hash copy k frame kva : %d\n", k->frame->kva);
			//printf("spt hash copy p frame kva : %d\n", p->frame->kva);
			memcpy(k->frame->kva, p->frame->kva, PGSIZE);
			//printf("spt hash copy memcpy 성공\n");
		}

		// struct page *new_page = malloc(sizeof(struct page));
		// struct page *p = hash_entry(hash_cur(&i), struct page, hash_elem);
		// uninit_new(new_page, p->va, p->uninit.init, p->operations->type, p->uninit.aux, p->uninit.page_initializer);
		// new_page->writable = p->writable;
		// if(!spt_insert_page(dst, new_page)){
		// 	//printf("spt hash copy spt insert page fail\n");
		// 	return false;
		// }
		// if(!vm_do_claim_page(new_page)){
		// 	//printf("spt hash copy vm do claim page fail\n");
		// 	return false;
		// }
	}
	return true;

}

/* Free the resource hold by the supplemental page table */
void
spt_hash_kill (struct hash *spt UNUSED) {
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
	// struct hash_iterator i;
	// hash_first(&i, spt);
	// while(hash_next(&i)){
	// 	struct page *p = hash_entry(hash_cur(&i), struct page, hash_elem);
	// 	destroy(p);
	// 	}

	hash_clear(spt, page_entry_destroy);
}

void
page_entry_destroy(struct hash_elem *e, void *aux){

	struct page *p = hash_entry(e, struct page, hash_elem);

	destroy(p);
	free(p);

}


uint64_t
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
	struct hash spt = thread_current()->spt;
  	p.va = address;
  	e = hash_find (&spt, &p.hash_elem);
  	return e != NULL ? hash_entry (e, struct page, hash_elem) : NULL;
}

