#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "intrinsic.h"
#include "threads/init.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "threads/palloc.h"


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

	lock_init(&filesys_lock);
}



/* The main system call interface */
void
syscall_handler (struct intr_frame *f) {
	check_address(f->rip);

	switch (f->R.rax)
	{
	case SYS_HALT:
		halt();
		break;
	case SYS_EXIT:
		exit((int)f->R.rdi);
		break;

	case SYS_FORK:
		fork((const char *)f->R.rdi);
		break;

	case SYS_EXEC:
		exec((const char *)f->R.rdi);
		break;
	
	case SYS_WAIT:
		wait((pid_t)f->R.rdi);
		break;
	
	case SYS_CREATE:
		create((const char *)f->R.rdi, (unsigned int) f->R.rsi);
		break;
	
	case SYS_REMOVE:
		remove((const char *)f->R.rdi);
		break;

	case SYS_OPEN:
		open((const char *)f->R.rdi);
		break;

	case SYS_FILESIZE:
		filesize((int)f->R.rdi);
		break;
	
	case SYS_READ:
		read((int)f->R.rdi, (void *)f->R.rsi, (unsigned int) f->R.rdx);
		break;

	case SYS_WRITE:
		write((int)f->R.rdi, (const void *)f->R.rsi, (unsigned int)f->R.rdx);
		break;

	case SYS_SEEK:
		seek((int)f->R.rdi, (unsigned int)f->R.rsi);
		break;
	
	case SYS_TELL:
		tell((int)f->R.rdi);
		break;

	case SYS_CLOSE:
		close((int)f->R.rdi);
		break;
	default:
		exit(-1);
		break;
	}

	thread_exit ();
}

void
check_address(void *addr){

	if(is_user_vaddr(addr)){
		
		if(pml4_get_page(thread_current()->pml4, addr) == NULL){
			exit(-1);
		}
	}
	else{
		exit(-1);
	}
}

void
halt (void) {
	power_off();
}

void
exit (int status) {
	// pid에 exit status 저장
	// 딱히 pid 가 없어서 thread... 안에 저장할까싶다
	thread_current()->exit_status = EXIT_SUCCESS;
	printf("%s: exit(%d)\n", thread_current()->name, status);
	thread_exit();
}

pid_t
fork (const char *thread_name){


}

int
exec (const char *file) {


}

int
wait (pid_t pid) {

}

bool
create (const char *file, unsigned initial_size) {
	lock_acquire(&filesys_lock);
	bool is_create = filesys_create(file, initial_size);
	lock_release(&filesys_lock);
	return is_create;

}

bool
remove (const char *file) {

	// 바로 삭제하지 않고 열려있다면 그 파일은 close가 되지 않도록 처리.
	lock_acquire(&filesys_lock);
	bool is_remove = filesys_remove(file);
	lock_release(&filesys_lock);
	return is_remove;

}

int
open (const char *file) {

	lock_acquire(&filesys_lock);
	struct file *open_n = filesys_open(file);
	lock_release(&filesys_lock);
	int new_fd = thread_current()->next_fd;
	thread_current()->fdt[new_fd] = open_n;
	thread_current()->next_fd += 1;

	return new_fd;
}

int
filesize (int fd) {

	struct file *target_file = thread_current()->fdt[fd];
	lock_acquire(&filesys_lock);
	off_t size = file_length(target_file);
	lock_release(&filesys_lock);
	return size;
}

int
read (int fd, void *buffer, unsigned size) {


	if(fd == 0){
		int key = input_getc();
		return key;
	}
	if(fd <64){
		struct file *target_file = thread_current()->fdt[fd];
		lock_acquire(&filesys_lock);
		off_t byte_read =  file_read(target_file,buffer,size);
		lock_release(&filesys_lock);
		return byte_read;
		}
	else{
		// 64 이상이나 이하의 fd를 가져왔다면 리턴.
		// 또 target_file 자체가 존재하는지도 확인해야함.
	}


}


int
write (int fd, const void *buffer, unsigned size) {


	struct file *target_file = thread_current()->fdt[fd];
	lock_acquire(&filesys_lock);
	off_t byte_write = file_write(target_file,buffer,size);
	lock_release(&filesys_lock);
	return byte_write;

}

void
seek (int fd, unsigned position) {

}

unsigned
tell (int fd) {
}

void
close (int fd) {
}

int dup2(int oldfd, int newfd){

}