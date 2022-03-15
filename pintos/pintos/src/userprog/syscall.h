#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);

static struct semaphore file_sema;
void exit_process (int exit_code);

#endif /* userprog/syscall.h */
