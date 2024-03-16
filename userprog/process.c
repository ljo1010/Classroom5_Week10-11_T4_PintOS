#include "userprog/process.h"
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "userprog/gdt.h"
#include "userprog/tss.h"
#include "filesys/directory.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/flags.h"
#include "threads/init.h"
#include "threads/interrupt.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/mmu.h"
#include "threads/vaddr.h"
#include "intrinsic.h"
#include "lib/string.h"
#include "threads/synch.h"
#include "lib/stdio.h"
#include "devices/timer.h"

#ifdef VM
#include "vm/vm.h"
#endif

static void process_cleanup (void);
static bool load (const char *file_name, struct intr_frame *if_);
static void initd (void *f_name);
static void __do_fork (void *);

/* General process initializer for initd and other process. */
static void
process_init (void) {
	struct thread *current = thread_current ();
}

/* Starts the first userland program, called "initd", loaded from FILE_NAME.
 * The new thread may be scheduled (and may even exit)
 * before process_create_initd() returns. Returns the initd's
 * thread id, or TID_ERROR if the thread cannot be created.
 * Notice that THIS SHOULD BE CALLED ONCE. */
tid_t
process_create_initd (const char *file_name) {
	char *fn_copy;
	tid_t tid;

	/* Make a copy of FILE_NAME.
	 * Otherwise there's a race between the caller and load(). */
	fn_copy = palloc_get_page (0);
	if (fn_copy == NULL)
		return TID_ERROR;
	strlcpy (fn_copy, file_name, PGSIZE);

	char buf[128];
	// file naem 전달시 parse
	char *save_ptr;
	strlcpy(buf,fn_copy, sizeof(buf));
	char *f_name = strtok_r(buf, " ", &save_ptr);

	// printf("yes");
	// printf("%s", buf); // buf는 잘 들어갔음.
	// printf("f_name: %s\n", f_name); //f_name도 잘 들어갔음

	if (f_name == NULL) {
        palloc_free_page (fn_copy);
        return TID_ERROR; // 파일 이름이 존재하지 않는 경우
		}

	/* Create a new thread to execute FILE_NAME. */
	tid = thread_create (f_name, PRI_DEFAULT, initd, fn_copy); // file_name ->token
	if (tid == TID_ERROR)
		palloc_free_page (fn_copy);

	


	return tid;
}

/* A thread function that launches first user process. */
static void
initd (void *f_name) {
#ifdef VM
	supplemental_page_table_init (&thread_current ()->spt);
#endif

	process_init ();

	if (process_exec (f_name) < 0)
		PANIC("Fail to launch initd\n");
	NOT_REACHED ();
}

/* Clones the current process as `name`. Returns the new process's thread id, or
 * TID_ERROR if the thread cannot be created. */
tid_t
process_fork (const char *name, struct intr_frame *if_ UNUSED) {



	/* Clone current thread to new thread.*/
	return thread_create (name,
			PRI_DEFAULT, __do_fork, thread_current ());
}

#ifndef VM
/* Duplicate the parent's address space by passing this function to the
 * pml4_for_each. This is only for the project 2. */
static bool
duplicate_pte (uint64_t *pte, void *va, void *aux) {
	struct thread *current = thread_current ();
	struct thread *parent = (struct thread *) aux;
	void *parent_page;
	void *newpage;
	bool writable;

	/* 1. TODO: If the parent_page is kernel page, then return immediately. */

	/* 2. Resolve VA from the parent's page map level 4. */
	parent_page = pml4_get_page (parent->pml4, va);

	/* 3. TODO: Allocate new PAL_USER page for the child and set result to
	 *    TODO: NEWPAGE. */

	/* 4. TODO: Duplicate parent's page to the new page and
	 *    TODO: check whether parent's page is writable or not (set WRITABLE
	 *    TODO: according to the result). */

	/* 5. Add new page to child's page table at address VA with WRITABLE
	 *    permission. */
	if (!pml4_set_page (current->pml4, va, newpage, writable)) {
		/* 6. TODO: if fail to insert page, do error handling. */
	}
	return true;
}
#endif

/* A thread function that copies parent's execution context.
 * Hint) parent->tf does not hold the userland context of the process.
 *       That is, you are required to pass second argument of process_fork to
 *       this function. */
static void
__do_fork (void *aux) {
	struct intr_frame if_;
	struct thread *parent = (struct thread *) aux;
	struct thread *current = thread_current ();
	/* TODO: somehow pass the parent_if. (i.e. process_fork()'s if_) */
	struct intr_frame *parent_if;
	bool succ = true;

	/* 1. Read the cpu context to local stack. */
	memcpy (&if_, parent_if, sizeof (struct intr_frame));

	/* 2. Duplicate PT */
	current->pml4 = pml4_create();
	if (current->pml4 == NULL)
		goto error;

	process_activate (current);
#ifdef VM
	supplemental_page_table_init (&current->spt);
	if (!supplemental_page_table_copy (&current->spt, &parent->spt))
		goto error;
#else
	if (!pml4_for_each (parent->pml4, duplicate_pte, parent))
		goto error;
#endif

	/* TODO: Your code goes here.
	 * TODO: Hint) To duplicate the file object, use `file_duplicate`
	 * TODO:       in include/filesys/file.h. Note that parent should not return
	 * TODO:       from the fork() until this function successfully duplicates
	 * TODO:       the resources of parent.*/

	process_init ();

	/* Finally, switch to the newly created process. */
	if (succ)
		do_iret (&if_);
error:
	thread_exit ();
}

/* Switch the current execution context to the f_name.
 * Returns -1 on fail. */
int
process_exec (void *f_name) {
	char *file_name = f_name;
	bool success;

	/* We cannot use the intr_frame in the thread structure.
	 * This is because when current thread rescheduled,
	 * it stores the execution information to the member. */
	struct intr_frame _if;
	_if.ds = _if.es = _if.ss = SEL_UDSEG;
	_if.cs = SEL_UCSEG;
	_if.eflags = FLAG_IF | FLAG_MBS;

	// printf("hie\n");
	// printf("process_exec file_name : %s\n", file_name); // 풀 네임인거 확인함.

	//------------------------------

	char *save_ptr;

	char full_f_name_buf[128];
	char full_name_buf[128];
	int count  = 0;
	strlcpy(full_f_name_buf, file_name,sizeof(full_f_name_buf)); // file_name 가져오기
	strlcpy(full_name_buf, file_name, sizeof(full_name_buf)); //strtok에서 잘리길래 두개 복사해놓음..

	// printf("process exec buf : %s\n", full_f_name_buf); // 잘 출력하는거 확인

	char f_name_arg[128];
	char *f_buf;

	for(f_buf = strtok_r(full_f_name_buf," ", &save_ptr); f_buf != NULL;f_buf = strtok_r(NULL, " ", &save_ptr)){
		strlcpy(f_name_arg, f_buf, sizeof(f_name_arg));
		// printf("process exec f_name_arg :%s\n", f_name_arg);
		count++;
	}// arg 갯수 세기


	//------------------------------

	/* We first kill the current context */
	process_cleanup ();

	/* And then load the binary */
	success = load (file_name, &_if);
	/* If load failed, quit. */
	palloc_free_page (file_name);
	if (!success)
		return -1;

	argument_passing(full_name_buf, count,(void **) &_if.rsp);

	_if.R.rdi = count;
	_if.R.rsi = _if.rsp+8;

	// hex_dump(_if.rsp, _if.rsp, USER_STACK - (uint64_t)_if.rsp, true); 

	/* Start switched process. */
	do_iret (&_if);
	NOT_REACHED ();
}


//포인터의 주소 자체를 수정해야해서, 포인터로 받아온게 아니라 이중 포인터로 받아옴.(포인터의 포인터로.)
void
argument_passing(char *file_name,int count,void **rsp){

	char full_f_name_buf[128];
	strlcpy(full_f_name_buf, file_name,sizeof(full_f_name_buf)); // file_name 가져오기
	// printf("argument passing file_name : %s\n", file_name); // 잘 출력하는거 확인
	// printf("argument passing full_f_name_buf : %s\n", full_f_name_buf); // 잘 출력하는거 확인

	char *f_buf, *save_ptr;
	int total_size = 0;
	char *parse[64];
	int count_parse = 0;

	// parse에 나눠서 arg별로 저장해두고
	for(f_buf = strtok_r(full_f_name_buf, " ", &save_ptr); f_buf != NULL; f_buf = strtok_r(NULL," ", &save_ptr)){
	
		parse[count_parse++] = f_buf;

	}

	// 한글자씩 1씩 낮춰가며 이동.
	for(int i = count-1; i > -1; i--){

		for(int j = strlen(parse[i]);j > -1; j--){

			(*rsp)--;
			**(char**)rsp=parse[i][j]; // char 형태로 넣어야해서 이중 포인터 안에 있는 값을 char 만큼 움직여 값 넣기.
			// printf("argument passing rsp : %p\n", &rsp);
			// printf("argument passing parse[i][j] : %p\n", &parse[i][j]); 

			total_size += 1;
		}
		parse[i] = *(char**)rsp; // 한번 카운트 다 돌때마다 맨 앞글자일테니 본래 자리를 주소로 갱신.
		// 주소라서 앞의 *갯수가 하나임.
	}

	if(total_size%8 != 0){
		size_t padding = 8 - (total_size%8);
		for(unsigned int k = 0; k<padding;k++){
			(*rsp)--;
			**(uint8_t**)rsp = 0;
		} // 패딩용, uin8_t만큼 움직이자.
		// printf("argument passing padding : %d\n", padding);

	}
	// align 용으로 8칸을 비워놓기 위함.
	(*rsp)-=8;
	**(char***)rsp = 0;
	// 근데 8칸 내내 0으로 채워놔야하는거아닌가? 왜 그 지점만 채워넣지?


	for (int i = count-1; i> -1; i--){

		(*rsp)-=8;
		**(char***)rsp = parse[i]; // char*만큼 움직이며 parse[i]를 넣기.
	}

	(*rsp)-=8;
	**(void***)rsp = 0;



}

/* Waits for thread TID to die and returns its exit status.  If
 * it was terminated by the kernel (i.e. killed due to an
 * exception), returns -1.  If TID is invalid or if it was not a
 * child of the calling process, or if process_wait() has already
 * been successfully called for the given TID, returns -1
 * immediately, without waiting.
 *
 * This function will be implemented in problem 2-2.  For now, it
 * does nothing. */
int
process_wait (tid_t child_tid) {
	/* XXX: Hint) The pintos exit if process_wait (initd), we recommend you
	 * XXX:       to add infinite loop here before
	 * XXX:       implementing the process_wait. */

	for (int i = 0; i < 100000000; i++)
	{
	}
	timer_sleep(10);
	printf("process wait 진입\n");
	struct list_elem *e;
	struct thread *curr = thread_current();
	printf("process wait curr name : %s\n",curr->name);
	printf("process wait child_tid : %d\n", child_tid);
	if(list_empty(&curr->child_list)){
		printf("비었구만욘\n");
	}
	for(e = list_begin(&curr->child_list); e != list_end(&curr->child_list); e = list_next(e)){
		struct thread *child = list_entry(e, struct thread , elem);
		printf("process wait 진입\n");
		printf("process wait child name : %s\n", child->name);
		if(child->tid == child_tid){
			sema_down(&curr->wait);
			return child->exit_status;
		}
	}
	
	return -1;
}

/* Exit the process. This function is called by thread_exit (). */
void
process_exit (void) {
	struct thread *curr = thread_current ();
	/* TODO: Your code goes here.
	 * TODO: Implement process termination message (see
	 * TODO: project2/process_termination.html).
	 * TODO: We recommend you to implement process resource cleanup here. */
	process_cleanup ();
}

/* Free the current process's resources. */
static void
process_cleanup (void) {
	struct thread *curr = thread_current ();

#ifdef VM
	supplemental_page_table_kill (&curr->spt);
#endif

	uint64_t *pml4;
	/* Destroy the current process's page directory and switch back
	 * to the kernel-only page directory. */
	pml4 = curr->pml4;
	if (pml4 != NULL) {
		/* Correct ordering here is crucial.  We must set
		 * cur->pagedir to NULL before switching page directories,
		 * so that a timer interrupt can't switch back to the
		 * process page directory.  We must activate the base page
		 * directory before destroying the process's page
		 * directory, or our active page directory will be one
		 * that's been freed (and cleared). */
		curr->pml4 = NULL;
		pml4_activate (NULL);
		pml4_destroy (pml4);
	}
}

/* Sets up the CPU for running user code in the nest thread.
 * This function is called on every context switch. */
void
process_activate (struct thread *next) {
	/* Activate thread's page tables. */
	pml4_activate (next->pml4);

	/* Set thread's kernel stack for use in processing interrupts. */
	tss_update (next);
}

/* We load ELF binaries.  The following definitions are taken
 * from the ELF specification, [ELF1], more-or-less verbatim.  */

/* ELF types.  See [ELF1] 1-2. */
#define EI_NIDENT 16

#define PT_NULL    0            /* Ignore. */
#define PT_LOAD    1            /* Loadable segment. */
#define PT_DYNAMIC 2            /* Dynamic linking info. */
#define PT_INTERP  3            /* Name of dynamic loader. */
#define PT_NOTE    4            /* Auxiliary info. */
#define PT_SHLIB   5            /* Reserved. */
#define PT_PHDR    6            /* Program header table. */
#define PT_STACK   0x6474e551   /* Stack segment. */

#define PF_X 1          /* Executable. */
#define PF_W 2          /* Writable. */
#define PF_R 4          /* Readable. */

/* Executable header.  See [ELF1] 1-4 to 1-8.
 * This appears at the very beginning of an ELF binary. */
struct ELF64_hdr {
	unsigned char e_ident[EI_NIDENT];
	uint16_t e_type;
	uint16_t e_machine;
	uint32_t e_version;
	uint64_t e_entry;
	uint64_t e_phoff;
	uint64_t e_shoff;
	uint32_t e_flags;
	uint16_t e_ehsize;
	uint16_t e_phentsize;
	uint16_t e_phnum;
	uint16_t e_shentsize;
	uint16_t e_shnum;
	uint16_t e_shstrndx;
};

struct ELF64_PHDR {
	uint32_t p_type;
	uint32_t p_flags;
	uint64_t p_offset;
	uint64_t p_vaddr;
	uint64_t p_paddr;
	uint64_t p_filesz;
	uint64_t p_memsz;
	uint64_t p_align;
};

/* Abbreviations */
#define ELF ELF64_hdr
#define Phdr ELF64_PHDR

static bool setup_stack (struct intr_frame *if_);
static bool validate_segment (const struct Phdr *, struct file *);
static bool load_segment (struct file *file, off_t ofs, uint8_t *upage,
		uint32_t read_bytes, uint32_t zero_bytes,
		bool writable);

/* Loads an ELF executable from FILE_NAME into the current thread.
 * Stores the executable's entry point into *RIP
 * and its initial stack pointer into *RSP.
 * Returns true if successful, false otherwise. */
static bool
load (const char *file_name, struct intr_frame *if_) {
	struct thread *t = thread_current ();
	struct ELF ehdr;
	struct file *file = NULL;
	off_t file_ofs;
	bool success = false;
	int i;

	// printf("load 진입\n");
	// printf("load file_name :%s\n", file_name);

	char f_name_buf[128];
	// file naem 전달시 parse
	char *name_ptr;
	strlcpy(f_name_buf,file_name, sizeof(f_name_buf));
	char *f_name = strtok_r(f_name_buf, " ", &name_ptr);

	/* Allocate and activate page directory. */
	t->pml4 = pml4_create ();
	if (t->pml4 == NULL)
		goto done;
	process_activate (thread_current ());



	/* Open executable file. */
	file = filesys_open (f_name);
	if (file == NULL) {
		printf ("load: %s: open failed\n", f_name);
		goto done;
	}

	/* Read and verify executable header. */
	if (file_read (file, &ehdr, sizeof ehdr) != sizeof ehdr
			|| memcmp (ehdr.e_ident, "\177ELF\2\1\1", 7)
			|| ehdr.e_type != 2
			|| ehdr.e_machine != 0x3E // amd64
			|| ehdr.e_version != 1
			|| ehdr.e_phentsize != sizeof (struct Phdr)
			|| ehdr.e_phnum > 1024) {
		printf ("load: %s: error loading executable\n", file_name);
		goto done;
	}

	/* Read program headers. */
	file_ofs = ehdr.e_phoff;
	for (i = 0; i < ehdr.e_phnum; i++) {
		struct Phdr phdr;

		if (file_ofs < 0 || file_ofs > file_length (file))
			goto done;
		file_seek (file, file_ofs);

		if (file_read (file, &phdr, sizeof phdr) != sizeof phdr)
			goto done;
		file_ofs += sizeof phdr;
		switch (phdr.p_type) {
			case PT_NULL:
			case PT_NOTE:
			case PT_PHDR:
			case PT_STACK:
			default:
				/* Ignore this segment. */
				break;
			case PT_DYNAMIC:
			case PT_INTERP:
			case PT_SHLIB:
				goto done;
			case PT_LOAD:
				if (validate_segment (&phdr, file)) {
					bool writable = (phdr.p_flags & PF_W) != 0;
					uint64_t file_page = phdr.p_offset & ~PGMASK;
					uint64_t mem_page = phdr.p_vaddr & ~PGMASK;
					uint64_t page_offset = phdr.p_vaddr & PGMASK;
					uint32_t read_bytes, zero_bytes;
					if (phdr.p_filesz > 0) {
						/* Normal segment.
						 * Read initial part from disk and zero the rest. */
						read_bytes = page_offset + phdr.p_filesz;
						zero_bytes = (ROUND_UP (page_offset + phdr.p_memsz, PGSIZE)
								- read_bytes);
					} else {
						/* Entirely zero.
						 * Don't read anything from disk. */
						read_bytes = 0;
						zero_bytes = ROUND_UP (page_offset + phdr.p_memsz, PGSIZE);
					}
					if (!load_segment (file, file_page, (void *) mem_page,
								read_bytes, zero_bytes, writable))
						goto done;
				}
				else
					goto done;
				break;
		}
	}

	/* Set up stack. */
	if (!setup_stack (if_))
		goto done;

	/* Start address. */
	if_->rip = ehdr.e_entry;




	// //file name을 명령 인수 등과 나눠서 user stack에 push
	// char *save_ptr;
	// struct list file_arg;
	// list_init(&file_arg);

	// char full_f_name_buf[128];
	// int count  = 0;
	// strlcpy(full_f_name_buf, file_name,sizeof(full_f_name_buf));

	// printf("load buf : %s\n", full_f_name_buf);

	// char f_name_arg[128];
	// char *f_buf;
	// int count = 0;

	// for(f_buf = strtok_r(full_f_name_buf," ", &save_ptr); f_buf != NULL;f_buf = strtok_r(NULL, " ", &save_ptr)){
	// 	strlcpy(f_name_arg, f_buf, sizeof(f_name_arg));
	// 	printf("load f_name_arg :%s\n", f_name_arg);
	// 	count++;
	// }

	// for()

	// for(token.s = strtok_r(full_f_name_buf, " ", &save_ptr);token.s != NULL; token.s = strtok_r(NULL, " ", &save_ptr)){
	// 	list_push_front(&file_arg, &token.elem);
    // 	snprintf(token.name, sizeof(token.name), "%d", count);
	// 	printf("load token.s : %s\n", token.s);
	// 	count += 1;
	// }


	// printf("스택 안에 넣어볼것임. \n");

	// printf("%d개가 들어왔군.\n", count);
	// struct list_elem *e;
	// size_t data_size = 0;
	// if (list_empty(&file_arg)) {
	// 	printf("file_arg is empty\n");
	// } else {
	// 	printf("file_arg is not empty\n");
	// }

	// for(e =list_begin(&file_arg);e != list_end(&file_arg);){
	// 	struct token *to= list_entry(e, struct token, elem);
	// 	printf("List element: %s\n", to->s);
	// 	char *s = to->s;
	// 	size_t word_size = strlen(s) + 1; // 널문자 포함으로 +1
	// 	data_size+= word_size; // padding용 총 길이 재기

	// 	to->addr = (char *)if_->rsp; // 현재 data 주소값 저장
		
	// 	// 이거쓰짖말래
	// 	// 그럼 그 주소에 값 s를 넣는건 어떻게 해야하지.... 
	// 	memcpy((void *)if_->rsp, s, word_size);	
	// 	if_->rsp -= word_size;
	// 	printf("뭐라도 해봐\n");
	// 	printf("Stack pointer after moving: %p\n", (void *)if_->rsp);
	// 	e = list_next(e);
	// 	break;
	// }
	// printf("for 넘어옴\n");

	// if(data_size%8 != 0){
	// 	// padding 해서 하는 만큼 push.
	// 	size_t padding_size = 8 - (data_size%8);
	// 	memset((void *)if_->rsp, 0, padding_size);
	// 	if_->rsp -= padding_size;
	// }

	// // 마지막 인자 null용 char * push.
	// *((char **)if_->rsp) = NULL;
	// if_->rsp -= sizeof(char *);

	// // arg의 주소값을 push
	// for(e = list_back(&file_arg);e != list_begin(&file_arg);e = list_prev(e)){
	// 	struct token *to= list_entry(e, struct token, elem);

	// 	*((char **)if_->rsp) = to->addr;
	// 	if_->rsp -= sizeof(char *);
	// }

	// //fake return addr을 push	
	// *((void **)if_->rsp) = NULL;
	// if_->rsp -= sizeof(void *);

	success = true;

done:
	/* We arrive here whether the load is successful or not. */
	file_close (file);
	return success;
}


/* Checks whether PHDR describes a valid, loadable segment in
 * FILE and returns true if so, false otherwise. */
static bool
validate_segment (const struct Phdr *phdr, struct file *file) {
	/* p_offset and p_vaddr must have the same page offset. */
	if ((phdr->p_offset & PGMASK) != (phdr->p_vaddr & PGMASK))
		return false;

	/* p_offset must point within FILE. */
	if (phdr->p_offset > (uint64_t) file_length (file))
		return false;

	/* p_memsz must be at least as big as p_filesz. */
	if (phdr->p_memsz < phdr->p_filesz)
		return false;

	/* The segment must not be empty. */
	if (phdr->p_memsz == 0)
		return false;

	/* The virtual memory region must both start and end within the
	   user address space range. */
	if (!is_user_vaddr ((void *) phdr->p_vaddr))
		return false;
	if (!is_user_vaddr ((void *) (phdr->p_vaddr + phdr->p_memsz)))
		return false;

	/* The region cannot "wrap around" across the kernel virtual
	   address space. */
	if (phdr->p_vaddr + phdr->p_memsz < phdr->p_vaddr)
		return false;

	/* Disallow mapping page 0.
	   Not only is it a bad idea to map page 0, but if we allowed
	   it then user code that passed a null pointer to system calls
	   could quite likely panic the kernel by way of null pointer
	   assertions in memcpy(), etc. */
	if (phdr->p_vaddr < PGSIZE)
		return false;

	/* It's okay. */
	return true;
}

#ifndef VM
/* Codes of this block will be ONLY USED DURING project 2.
 * If you want to implement the function for whole project 2, implement it
 * outside of #ifndef macro. */

/* load() helpers. */
static bool install_page (void *upage, void *kpage, bool writable);

/* Loads a segment starting at offset OFS in FILE at address
 * UPAGE.  In total, READ_BYTES + ZERO_BYTES bytes of virtual
 * memory are initialized, as follows:
 *
 * - READ_BYTES bytes at UPAGE must be read from FILE
 * starting at offset OFS.
 *
 * - ZERO_BYTES bytes at UPAGE + READ_BYTES must be zeroed.
 *
 * The pages initialized by this function must be writable by the
 * user process if WRITABLE is true, read-only otherwise.
 *
 * Return true if successful, false if a memory allocation error
 * or disk read error occurs. */
static bool
load_segment (struct file *file, off_t ofs, uint8_t *upage,
		uint32_t read_bytes, uint32_t zero_bytes, bool writable) {
	ASSERT ((read_bytes + zero_bytes) % PGSIZE == 0);
	ASSERT (pg_ofs (upage) == 0);
	ASSERT (ofs % PGSIZE == 0);

	file_seek (file, ofs);
	while (read_bytes > 0 || zero_bytes > 0) {
		/* Do calculate how to fill this page.
		 * We will read PAGE_READ_BYTES bytes from FILE
		 * and zero the final PAGE_ZERO_BYTES bytes. */
		size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
		size_t page_zero_bytes = PGSIZE - page_read_bytes;

		/* Get a page of memory. */
		uint8_t *kpage = palloc_get_page (PAL_USER);
		if (kpage == NULL)
			return false;

		/* Load this page. */
		if (file_read (file, kpage, page_read_bytes) != (int) page_read_bytes) {
			palloc_free_page (kpage);
			return false;
		}
		memset (kpage + page_read_bytes, 0, page_zero_bytes);

		/* Add the page to the process's address space. */
		if (!install_page (upage, kpage, writable)) {
			printf("fail\n");
			palloc_free_page (kpage);
			return false;
		}

		/* Advance. */
		read_bytes -= page_read_bytes;
		zero_bytes -= page_zero_bytes;
		upage += PGSIZE;
	}
	return true;
}

/* Create a minimal stack by mapping a zeroed page at the USER_STACK */
static bool
setup_stack (struct intr_frame *if_) {
	uint8_t *kpage;
	bool success = false;

	kpage = palloc_get_page (PAL_USER | PAL_ZERO);
	if (kpage != NULL) {
		success = install_page (((uint8_t *) USER_STACK) - PGSIZE, kpage, true);
		if (success)
			if_->rsp = USER_STACK;
		else
			palloc_free_page (kpage);
	}
	return success;
}

/* Adds a mapping from user virtual address UPAGE to kernel
 * virtual address KPAGE to the page table.
 * If WRITABLE is true, the user process may modify the page;
 * otherwise, it is read-only.
 * UPAGE must not already be mapped.
 * KPAGE should probably be a page obtained from the user pool
 * with palloc_get_page().
 * Returns true on success, false if UPAGE is already mapped or
 * if memory allocation fails. */
static bool
install_page (void *upage, void *kpage, bool writable) {
	struct thread *t = thread_current ();

	/* Verify that there's not already a page at that virtual
	 * address, then map our page there. */
	return (pml4_get_page (t->pml4, upage) == NULL
			&& pml4_set_page (t->pml4, upage, kpage, writable));
}
#else
/* From here, codes will be used after project 3.
 * If you want to implement the function for only project 2, implement it on the
 * upper block. */

static bool
lazy_load_segment (struct page *page, void *aux) {
	/* TODO: Load the segment from the file */
	/* TODO: This called when the first page fault occurs on address VA. */
	/* TODO: VA is available when calling this function. */
}

/* Loads a segment starting at offset OFS in FILE at address
 * UPAGE.  In total, READ_BYTES + ZERO_BYTES bytes of virtual
 * memory are initialized, as follows:
 *
 * - READ_BYTES bytes at UPAGE must be read from FILE
 * starting at offset OFS.
 *
 * - ZERO_BYTES bytes at UPAGE + READ_BYTES must be zeroed.
 *
 * The pages initialized by this function must be writable by the
 * user process if WRITABLE is true, read-only otherwise.
 *
 * Return true if successful, false if a memory allocation error
 * or disk read error occurs. */
static bool
load_segment (struct file *file, off_t ofs, uint8_t *upage,
		uint32_t read_bytes, uint32_t zero_bytes, bool writable) {
	ASSERT ((read_bytes + zero_bytes) % PGSIZE == 0);
	ASSERT (pg_ofs (upage) == 0);
	ASSERT (ofs % PGSIZE == 0);

	while (read_bytes > 0 || zero_bytes > 0) {
		/* Do calculate how to fill this page.
		 * We will read PAGE_READ_BYTES bytes from FILE
		 * and zero the final PAGE_ZERO_BYTES bytes. */
		size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
		size_t page_zero_bytes = PGSIZE - page_read_bytes;

		/* TODO: Set up aux to pass information to the lazy_load_segment. */
		void *aux = NULL;
		if (!vm_alloc_page_with_initializer (VM_ANON, upage,
					writable, lazy_load_segment, aux))
			return false;

		/* Advance. */
		read_bytes -= page_read_bytes;
		zero_bytes -= page_zero_bytes;
		upage += PGSIZE;
	}
	return true;
}

/* Create a PAGE of stack at the USER_STACK. Return true on success. */
static bool
setup_stack (struct intr_frame *if_) {
	bool success = false;
	void *stack_bottom = (void *) (((uint8_t *) USER_STACK) - PGSIZE);

	/* TODO: Map the stack on stack_bottom and claim the page immediately.
	 * TODO: If success, set the rsp accordingly.
	 * TODO: You should mark the page is stack. */
	/* TODO: Your code goes here */

	return success;
}
#endif /* VM */
