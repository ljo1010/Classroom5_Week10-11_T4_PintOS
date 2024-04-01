/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "threads/thread.h"
#include "threads/mmu.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "userprog/process.h"


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
	list_init(&frame_table);
	lock_init(&frame_table_lock);
}

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. */
enum vm_type
page_get_type (struct page *page) 
{
	int ty = VM_TYPE (page->operations->type);
	switch (ty) 
	{
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



// unsigned hash_page(const struct hash_elem *p_, void *aux);
// bool hash_less(const struct hash_elem *a_, const struct hash_elem *b_, void *aux UNUSED);


/* Create the pending page object with initializer. If you want to create a
 * page, do not create it directly and make it through this function or
 * `vm_alloc_page`. */
//initalizer를 이용해서 보류 페이지를 만들기. 만약 페이지 만들기를 원한다면, 
// 직접적으로 만들기말고 함수를 가져오기. 'vm_alloc_page' 
bool
vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable,
		vm_initializer *init, void *aux) {


	ASSERT (VM_TYPE(type) != VM_UNINIT)

	struct supplemental_page_table *spt = &thread_current ()->spt;

	/* Check wheter the upage is already occupied or not. */
	if (spt_find_page (spt, upage) == NULL) {
		/* TODO: Create the page, fetch the initialier according to the VM type,
		 * TODO: and then create "uninit" page struct by calling uninit_new. You
		 * TODO: should modify the field after calling the uninit_new. */
		//page table 추가해주기
		struct page *page= (struct page *)malloc(sizeof(struct page));
		/* TODO: Insert the page into the spt. */
		bool (*page_initializer)(struct page *, enum vm_type, void *);
		
		switch (VM_TYPE(type))
		{
		case VM_ANON:
			page_initializer = anon_initializer;		
			break;
		case VM_FILE:
			page_initializer = file_backed_initializer;
			break;
		}

		uninit_new(page, upage, init, type, aux, page_initializer);
		page->writable = writable;

		return spt_insert_page(spt, page);
	}
err:
	return false;
}

/* Find VA from spt and return page. On error, return NULL. */
struct page *
spt_find_page (struct supplemental_page_table *spt UNUSED, void *va UNUSED) {

	struct page *page = NULL;
	/* TODO: Fill this function. */
	page = (struct page *)malloc(sizeof(struct page));
	struct hash_elem *e;

	// va에 해당하는 hash_elem 찾기
	page->va = pg_round_down(va); // page의 시작 주소 할당
	e = hash_find(&spt->spt_table, &page->hash_elem);
	free(page);
	// printf("잘 드러가는 지 확인그%d\n", &e);
	// 있으면 e에 해당하는 페이지 반환
	return e != NULL ? hash_entry(e, struct page, hash_elem) : NULL;
	// printf("잘 드러가는 지 확인그%d\n", &e);
	
	}

/* Insert PAGE into spt with validation. */
bool
spt_insert_page (struct supplemental_page_table *spt UNUSED,
		struct page *page UNUSED) {
	/* TODO: Fill this function. */
	return hash_insert(&spt->spt_table, &page->hash_elem) == NULL ? true : false;
}

void
spt_remove_page (struct supplemental_page_table *spt, struct page *page) {
	
	hash_delete(&spt->spt_table, &page->hash_elem);
	vm_dealloc_page (page);
	return true;
}

/* Get the struct frame, that will be evicted. */
static struct frame *
vm_get_victim (void) {
	struct frame *victim = NULL;
	 /* TODO: The policy for eviction is up to you. */
	struct thread *curr = thread_current();

	lock_acquire(&frame_table_lock);
	struct list_elem *start = list_begin(&frame_table);
	for (start; start != list_end(&frame_table); start = list_next(start))
	{
		victim = list_entry(start, struct frame, frame_elem);
		if (victim->page == NULL) // frame에 할당된 페이지가 없는 경우 (page가 destroy된 경우 )
		{
			lock_release(&frame_table_lock);
			return victim;
		}
		if (pml4_is_accessed(curr->pml4, victim->page->va))
			pml4_set_accessed(curr->pml4, victim->page->va, 0);
		else
		{
			lock_release(&frame_table_lock);
			return victim;
		}
	}
	lock_release(&frame_table_lock);
	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame (void) {
	struct frame *victim UNUSED = vm_get_victim ();
	/* TODO: swap out the victim and return the evicted frame. */
	if(victim->page)
		swap_out(victim->page);
	return victim;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
static struct frame *
vm_get_frame (void) {
	struct frame *frame = NULL;
	/* TODO: Fill this function. */
	void *kva = palloc_get_page(PAL_USER);

	if(kva == NULL)
	{
		struct frame *victim = vm_evict_frame();
		victim->page = NULL;
		return victim;
	}

	frame = (struct frame *)malloc(sizeof(struct frame));
	frame->kva = kva;
	frame->page = NULL;

	lock_acquire(&frame_table_lock);
	list_push_back(&frame_table, &frame->frame_elem);
	lock_release(&frame_table_lock);
	ASSERT (frame != NULL);
	ASSERT (frame->page == NULL);
	return frame;
}

/* Growing the stack. */
static void
vm_stack_growth (void *addr UNUSED)
{
	vm_alloc_page(VM_ANON | VM_MARKER_0, pg_round_down(addr), 1);
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp (struct page *page UNUSED) 
{

}

/* Return true on success */
/*각각의 인자들 설명
1. 인터럽트를 처리하기 위한 구조체 포안터
2. 페이지 부재가 발생한 가상 주소를 나타냄
3. 페이지 부재가 사용자 모드에서 발생했는지 나타내는 bool값
4. 페이지 부재가 쓰기 작업으로 인한 것인지 나타내는 bool값
5. 페이지 부재가 발생한 페이지가 현재 메모리에 없는 상태인지를 나타내는 bool값*/
bool
vm_try_handle_fault (struct intr_frame *f UNUSED, void *addr UNUSED,
		bool user UNUSED, bool write UNUSED, bool not_present UNUSED) {

	struct supplemental_page_table *spt UNUSED = &thread_current()->spt;
	struct page *page = NULL;
	if (addr == NULL)
		return false;

	if (is_kernel_vaddr(addr))
		return false;

	if (not_present) // 접근한 메모리의 physical page가 존재하지 않은 경우
	{
		/* TODO: Validate the fault */
		// todo: 페이지 폴트가 스택 확장에 대한 유효한 경우인지를 확인해야 합니다.
		void *rsp = f->rsp; // user access인 경우 rsp는 유저 stack을 가리킨다.
		if (!user)			// kernel access인 경우 thread에서 rsp를 가져와야 한다.
			rsp = thread_current()->rsp;

		// 스택 확장으로 처리할 수 있는 폴트인 경우, vm_stack_growth를 호출
		if ((USER_STACK - (1 << 20) <= rsp - 8 && rsp - 8 == addr && addr <= USER_STACK) || (USER_STACK - (1 << 20) <= rsp && rsp <= addr && addr <= USER_STACK))
			vm_stack_growth(addr);

		page = spt_find_page(spt, addr);
		if (page == NULL)
			return false;
		if (write == 1 && page->writable == 0) // write 불가능한 페이지에 write 요청한 경우
			return false;
		return vm_do_claim_page(page);
	}
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
	page = spt_find_page(&thread_current()->spt, va);
	if(page == NULL)
	{
		return false;
	}

	return vm_do_claim_page (page);
}

/* Claim the PAGE and set up the mmu. */
//인자로 준 page에 물리적 프레임을 할당한다.
//vm_get_frame에서 함수를 불러와 프레임을 가져온다.(가상 주소와 물리 주소 매핑을 통해 페이지 테이블을 추가) 
static bool
vm_do_claim_page (struct page *page) {
	struct frame *frame = vm_get_frame();

	/* Set links */
	frame->page = page;
	page->frame = frame;

	/* TODO: Insert page table entry to map page's VA to frame's PA. */
	struct thread *current = thread_current();
	pml4_set_page(current->pml4, page->va, frame->kva, page->writable);

	return swap_in (page, frame->kva);
}

unsigned hash_page(const struct hash_elem *p_, void *aux)
{
	const struct page *p = hash_entry(p_, struct page, hash_elem);
	return hash_bytes(&p->va, sizeof p->va);
}

bool hash_less(const struct hash_elem *a_, const struct hash_elem *b_, void *aux UNUSED)
{
	const struct page *a = hash_entry(a_, struct page, hash_elem);
	const struct page *b = hash_entry(b_, struct page, hash_elem);

	return a->va < b->va;
}
/* Initialize new supplemental page table */
void
supplemental_page_table_init (struct supplemental_page_table *spt UNUSED) {
 
	hash_init(&spt->spt_table, hash_page, hash_less, NULL);
}


/* Copy supplemental page table from src to dst */
bool supplemental_page_table_copy (struct supplemental_page_table *dst UNUSED, struct supplemental_page_table *src UNUSED) 
{
	struct hash_iterator iter;
	hash_first(&iter, &src->spt_table);
	while (hash_next(&iter))
	{
		//소스들이 가져야하는 정보들
		struct page *src_p = hash_entry(hash_cur(&iter), struct page, hash_elem);
		enum vm_type type = src_p->operations->type;
		void *upage = src_p->va;
		bool writable = src_p->writable;

		if(type == VM_UNINIT)
		{
			//소스 타입이 VM_UNINIT일때
			//uninit을 만들고 초기화 애야한다. 
			vm_initializer *init = src_p->uninit.init;
			void *aux = src_p->uninit.aux;
			vm_alloc_page_with_initializer(VM_ANON, upage, writable, init, aux);
			continue;
		}

		if (type == VM_FILE)
        {
            struct lazy_load_arg *file_aux = malloc(sizeof(struct lazy_load_arg));
            file_aux->file = src_p->file.file;
            file_aux->ofs = src_p->file.ofs;
            file_aux->read_bytes = src_p->file.read_bytes;
			file_aux->zero_bytes = src_p->file.zero_bytes;
            if (!vm_alloc_page_with_initializer(type, upage, writable, NULL, file_aux))
                return false;
            struct page *file_page = spt_find_page(dst, upage);
            file_backed_initializer(file_page, type, NULL);
            file_page->frame = src_p->frame;
            pml4_set_page(thread_current()->pml4, file_page->va, src_p->frame->kva, src_p->writable);
            continue;
        }
		
		if(!vm_alloc_page(type, upage, writable))
		{
			return false;
		}

		if(!vm_claim_page(upage))
		{
			return false;
		}
		struct page *dst_p = spt_find_page(dst, upage);
		memcpy(dst_p->frame->kva, src_p->frame->kva, PGSIZE);
	}
	return true;
}
/* Free the resource hold by the supplemental page table */
void supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) 
{
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */

	hash_clear(&spt->spt_table, hash_page_destory);
}

void hash_page_destory(struct hash_elem *e, void *aux)
{
	struct page *page = hash_entry(e, struct page, hash_elem);

	destroy(page);
	free(page);

}