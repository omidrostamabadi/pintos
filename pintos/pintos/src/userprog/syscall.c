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
      exit_handler(f);
      break;
    
    case SYS_PRACTICE:
      practice_handler (f);
      break;

    case SYS_FILESIZE:
       filesize_handler (f);
       break;

    case SYS_CLOSE:
       close_handler (f);
       break;

    case SYS_TELL:
       tell_handler (f);
       break;

    case SYS_SEEK:
       seek_handler (f);
       break;

    case SYS_HALT:
      halt_handler (f);
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

static void halt_handler (struct intr_frame *f)
{
  uint32_t* args = ((uint32_t*) f->esp);
  shut_down_power_off (); // Defined in shutdown.c
}

static void practice_handler (struct intr_frame *f)
{
  uint32_t* args = ((uint32_t*) f->esp);
  /* Make sure args[1] is valid */
  bool arg_valid = is_valid_ptr (args[1], 4);
  /* Exit and free resources if not valid */
  if (!arg_valid)
    exit_process (-1);
  f->eax = args[1] + 1;
}

static void filesize_handler (struct intr_frame *f)
{
  uint32_t* args = ((uint32_t*) f->esp);
  /* Make sure args[1] is valid */
  bool arg_valid = is_valid_ptr (args[1], 4);
  /* Exit and free resources if not valid */
  if (!arg_valid)
    exit_process (-1);
  struct file *file = get_file_from_fd (args[1]);
  if (file != NULL)
    {
      sema_down (&file_sema); // Acquire global filesystem lock
      f->eax = file_length (file);
      sema_up (&file_sema); // Release global filesystem lock
    }
  else
    {
      exit_process (-1);
    }
}

static void close_handler (struct intr_frame *f)
{
  uint32_t* args = ((uint32_t*) f->esp);
  /* Make sure args[1] is valid */
  bool arg_valid = is_valid_ptr (args[1], 4);
  /* Exit and free resources if not valid */
  if (!arg_valid)
    exit_process (-1);

  struct file *file = get_file_from_fd (args[1]);
  if (file != NULL)
    {
      sema_down (&file_sema); // Acquire global filesystem lock
      file_close (file);
      sema_up (&file_sema); // Release global filesystem lock

      /* Remove file from open_files list of this thread */
      struct thread *current = thread_current ();
      struct list_elem *e;
      for (e = list_begin (&current->open_files); e != list_end (&current->open_files);
      e = list_next (e))
        {
          struct open_file *of = list_entry (e, struct open_file, file_elem);
          if (of->fd == args[1])
            {
              list_remove (e);
              free (of);
              break; // Break for loop
            }
        }
    }
  else
    {
      exit_process (-1);
    }
}

static void tell_handler (struct intr_frame *f)
{
  uint32_t* args = ((uint32_t*) f->esp);
  /* Make sure args[1] is valid */
  bool arg_valid = is_valid_ptr (args[1], 4);
  /* Exit and free resources if not valid */
  if (!arg_valid)
    exit_process (-1);

  struct file *file = get_file_from_fd (args[1]);
  if (file != NULL)
    {
      sema_down (&file_sema); // Acquire global filesystem lock
      f->eax = file_tell (file);
      sema_up (&file_sema); // Release global filesystem lock
    }
  else
    {
      exit_process (-1);
    }
}

static void seek_handler (struct intr_frame *f)
{
  uint32_t* args = ((uint32_t*) f->esp);
  /* Make sure args[1] & args[2] are valid */
  bool arg_valid = is_valid_ptr (args[1], 8);
  /* Exit and free resources if not valid */
  if (!arg_valid)
    exit_process (-1);

  struct file *file = get_file_from_fd (args[1]);
  if (file != NULL)
    {
      sema_down (&file_sema); // Acquire global filesystem lock
      uint32_t f_size = file_length (file);
      sema_up (&file_sema); // Release global filesystem lock
      int file_pos = (int) args[2];
      if (file_pos < 0)
      {
        exit_process (-1);
      }
      if (args[2] < f_size)
      {
        sema_down (&file_sema); // Acquire global filesystem lock
        file_seek (file, args[2]);
        sema_up (&file_sema); // Release global filesystem lock
      }
      else
      {
        exit_process (-1);
      }
    }
  else
    {
      exit_process (-1);
    }
}

/* Handle exec Syscall */
static void exec_handler(struct intr_frame *f){
  char* command_line = f->esp+1;
  //if (!is_valid_ptr(command_line))
    //exit_process(-1);
  sema_down (&file_sema);
  tid_t tid = process_execute (command_line);
  sema_up (&file_sema);
  
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

/* Handle exit Syscall */
static void exit_handler(struct intr_frame *f){
  int exit_code = f->esp+1;
  f->eax = exit_code;
  struct thread *current_thread = thread_current ();
  current_thread->child->exit_code = exit_code;
  printf ("%s: exit(%d)\n", &thread_current ()->name, exit_code);
  thread_exit ();
}

/* Exit and kill current process */
static void exit_process (int exit_code)
{
  struct thread *current_thread = thread_current ();
  current_thread->child->exit_code = exit_code;
  printf ("%s: exit(%d)\n", current_thread->name, exit_code);
  thread_exit ();
}

/* Helper functions for syscall handlers */

/* Iterate over current thread's list of open files
   and return file * that matches fd
   If no file exists with given fd, return NULL */
static struct file *
get_file_from_fd (int fd)
{
    struct thread *current = thread_current ();
    struct list_elem *e;
    for (e = list_begin (&current->open_files); e != list_end (&current->open_files);
         e = list_next (e))
    {
        struct open_file *of = list_entry (e, struct open_file, file_elem);
        if (of->fd == fd)
        {
            return of->this_file;
        }
    }

    /* When reach here, fd cannot be found */
    return NULL;
}