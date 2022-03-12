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

  switch (args[0])
    {
    case SYS_EXIT:
      f->eax = args[1];
      printf ("%s: exit(%d)\n", &thread_current ()->name, args[1]);
      thread_exit ();
      break;
    
    case SYS_PRACTICE:
      //bool arg_valid = is_valid_ptr(args[1], 4);
      f->eax = args[1] + 1; // Should validate args[1] address!
      break;

    case SYS_FILESIZE:
      //bool arg_valid = is_valid_ptr(args[1], 4);
      //if arg_valid fails, should take correct action here (kill and free)
      struct file *file = get_file_from_fd(args[1]);
      if(file != NULL)
       {
         sema_down(&file_sema); // Acquire global filesystem lock
         f->eax = file_length(file);
         sema_up(&file_sema); // Release global filesystem lock
       }
      else
       {
        //f->eax = file_not_found; or should kill the process?
       }
       break;

    case SYS_CLOSE:
      //bool arg_valid = is_valid_ptr(args[1], 4);
      //if arg_valid fails, should take correct action here (kill and free)
      struct file *file = get_file_from_fd(args[1]);
      if(file != NULL)
       {
         sema_down(&file_sema); // Acquire global filesystem lock
         file_close(file);
         sema_up(&file_sema); // Release global filesystem lock

         /* Remove file from open_files list of this thread */
         struct thread *current = thread_current();
         struct list_elem *e;
         for (e = list_begin (&current->open_files); e != list_end (&current->open_files);
          e = list_next (e))
          {
            struct open_file *of = list_entry (e, struct open_file, file_elem);
            if(of->fd == args[1])
             {
               list_remove(e);
               free(of);
               break; // Break for loop
             }
          }
       }
      else
       {
        //f->eax = file_not_found; or should kill the process?
       }
       break;
    
    case SYS_TELL:
      //bool arg_valid = is_valid_ptr(args[1], 4);
      //if arg_valid fails, should take correct action here (kill and free)
      struct file *file = get_file_from_fd(args[1]);
      if(file != NULL)
       {
         sema_down(&file_sema); // Acquire global filesystem lock
         f->eax = file_tell(file);
         sema_up(&file_sema); // Release global filesystem lock
       }
      else 
       {
         //f->eax = file_not_found; or should kill the process?
       }
       break;
    
    default:
      break;
    }
}
