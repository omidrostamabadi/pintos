#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "filesys/inode.h"
#include "filesys/directory.h"

static void syscall_handler (struct intr_frame *);
static struct semaphore file_sema;
struct file_descriptor{
    int fd_num,
    struct file* file,
    tid_t thread_number,

};
void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  sema_init(&file_sema, 1);
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

    case SYS_SEEK:
      //bool arg_valid = is_valid_ptr(args[1], 8); // It has two args, so validate 8 bytes
      //if arg_valid fails, should take correct action here (kill and free)
      struct file *file = get_file_from_fd(args[1]);
      if(file != NULL)
       {
         sema_down(&file_sema); // Acquire global filesystem lock
         uint32_t f_size = file_length(file);
         sema_up(&file_sema); // Release global filesystem lock
         int file_pos = (int) args[2];
         if(file_pos < 0) 
          {
            // kill thread (process) and free resources
          }
         if(args[2] < f_size) 
          {
            sema_down(&file_sema); // Acquire global filesystem lock
            file_seek(file, args[2]);
            sema_up(&file_sema); // Release global filesystem lock
          }
         else
          {
            // Kill the process, free resources and exit
          }
       }
      else 
       {
         //f->eax = file_not_found; or should kill the process?
       }
       break;

    case SYS_HALT:
      shut_down_power_off(); // Defined in shutdown.c
      break;

    case SYS_CREATE:
        f->eax = create_handler((const char*) args[1], (size_t) args[2]);
        break;
    case SYS_REMOVE:
        f->eax = remove_handler((const char*) args[1]);
        break;
    case SYS_READ:
        f->eax = write_handler((int) args[1], (const void*) args[2], (size_t) args[3]);
        break;
    case SYS_WRITE:
        f->eax = read_handler((int) args[1], (void*) args[2], (size_t) args[3]);
        break;
    default:
      break;
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

/* Handle wait Syscall */
static void wait_handler(struct intr_frame *f){
  tid_t child_tid = f->esp+1;
  f->eax = process_wait (child_tid);
}


static bool is_valid_pointer(){

}


static bool create_handler(const char* file_name, size_t size){
    //
    // validity_check if then exit
    //
    bool status = false;
    sema_up(&file_sema);
    status = filesys_create(file_name, size);
    sema_down(&file_sema);
    return status;
}

static bool remove_handler(const char* file_name){
    //
    // validity_check if then exit
    //
    bool status = false;
    sema_up(&file_sema);
    status = filesys_remove(file_name);
    sema_down(&file_sema);
    return status;
}

static int write_handler(int file_descriptor, const void* buffer, size_t buffer_size){

}

static int read_handler(int file_descriptor, void* buffer, size_t buffer_size){

}

static void check_read_buffer(){

}


static void check_write_buffer(){

}


