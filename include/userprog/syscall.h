#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H


#include <stdbool.h>
#include <debug.h>
#include <stddef.h>

/* Process identifier. */
typedef int pid_t;
#define PID_ERROR ((pid_t) -1)

/* Map region identifier. */
typedef int off_t;
#define MAP_FAILED ((void *) NULL)

/* Maximum characters in a filename written by readdir(). */
#define READDIR_MAX_LEN 14

/* Typical return values from main() and arguments to exit(). */
#define EXIT_SUCCESS 0          /* Successful execution. */
#define EXIT_FAILURE 1          /* Unsuccessful execution. */

void syscall_init (void);

void halt (void);
void exit (int status);
pid_t fork (const char *thread_name);
int exec (const char *cmd_line);
int wait (pid_t pid);
bool create (const char *file, unsigned initial_size);
bool remove (const char *file);
int open (const char *file);
int filesize (int fd);
int read (int fd, void *buffer, unsigned size);
int write (int fd, const void *buffer, unsigned size);
void seek (int fd, unsigned position);
unsigned tell (int fd);
void close (int fd);

#endif /* userprog/syscall.h */
