#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED)
{
  uint32_t* args = ((uint32_t*) f->esp);

  /*
   * The following print statement, if uncommented, will print out the syscall
   * number whenever a process enters a system call. You might find it useful
   * when debugging. It will cause tests to fail, however, so you should not
   * include it in your final submission.
   */

  /* printf("System call number: %d\n", args[0]); */

  if (args[0] == SYS_EXIT)
    {
      f->eax = args[1];
      printf ("%s: exit(%d)\n", &thread_current ()->name, args[1]);
      thread_exit ();
    }
}

/* Handle exec Syscall */
static void exec_handler(struct intr_frame *f){
  char* command_line = f->esp+1;
  //if (!is_valid_ptr(command_line))
    //exit(-1);
  //sema_down (&file_sema);  
  tid_t tid = process_execute (command_line);
  //sema_up (&file_sema);
  
  if (tid == TID_ERROR)
    {
      /* Process execution is failed */
      f->eax = -1;
      return;
    }

  struct thread *current_thread = thread_current ();
  struct list_elem *e;
  /* Trying to find Child elem with thread ID = tid */
  for (e = list_begin (&current_thread->children); e != list_end (&current_thread->children); e = list_next (e))
    {
      struct child *child = list_entry (e, struct child, elem);
      if (child->tid == tid)
        {
          sema_down (&child->wait_sem);
          if (!child->loaded_status)
            {
              /* Child loading was failed */
              f->eax = -1;
            }
          /* Child loaded successfully */  
          f->eax=tid:  
          return;
        }
    }
  /* Child thread not found */    
  f->eax = -1;
}
