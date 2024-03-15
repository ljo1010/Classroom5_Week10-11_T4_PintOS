#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "intrinsic.h"

void syscall_entry (void);
void syscall_handler (struct intr_frame *);

/* System call.
 *
 * Previously system call services was handled by the interrupt handler
 * (e.g. int 0x80 in linux). However, in x86-64, the manufacturer supplies
 * efficient path for requesting the system call, the `syscall` instruction.
 *
 * The syscall instruction works by reading the values from the the Model
 * Specific Register (MSR). For the details, see the manual. */

#define MSR_STAR 0xc0000081         /* Segment selector msr */
#define MSR_LSTAR 0xc0000082        /* Long mode SYSCALL target */
#define MSR_SYSCALL_MASK 0xc0000084 /* Mask for the eflags */

void
syscall_init (void) {
	write_msr(MSR_STAR, ((uint64_t)SEL_UCSEG - 0x10) << 48  |
			((uint64_t)SEL_KCSEG) << 32);
	write_msr(MSR_LSTAR, (uint64_t) syscall_entry);

	/* The interrupt service rountine should not serve any interrupts
	 * until the syscall_entry swaps the userland stack to the kernel
	 * mode stack. Therefore, we masked the FLAG_FL. */
	write_msr(MSR_SYSCALL_MASK,
			FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);
}



/* The main system call interface */
void
syscall_handler (struct intr_frame *f) {
	// TODO: Your implementation goes here.


	printf ("system call!\n");
	// 여기는 OS
	printf("syscall handler f->es : %x\n", f->es);
	printf("syscall handler f->ds : %x\n", f->ds);
	printf("syscall handler f->vec_no : %llx\n", f->vec_no);
	printf("syscall handler f->error_code : %llx\n", f->error_code);
	// 아래는 CPU
	printf("syscall handler f->rip : %llx\n", f->rip);
	printf("syscall handler f->cs : %x\n", f->cs);
	printf("syscall handler f->rsp : %llx\n", f->rsp);
	printf("syscall handler f->ss : %x\n", f->ss);
	printf("syscall handler f->eflags : %llx\n", f->eflags);

	switch (f->R.rax)
	{
	case /* constant-expression */:
		/* code */
		break;
	
	default:
		break;
	}

	// check_address(addr);

	thread_exit ();
}

void
check_address(void *addr){

	if(is_user_vaddr(addr)){
		
		if(pml4_get_page(thread_current()->pml4, addr) == NULL){
			// exit(-1);
		}
	}
	else{
		// exit(-1);
	}
}
