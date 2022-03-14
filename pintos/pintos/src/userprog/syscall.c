#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "userprog/pagedir.h"
#include "devices/shutdown.h"

static void syscall_handler (struct intr_frame *);

void exec_handler(struct intr_frame *f);
void wait_handler(struct intr_frame *f);
void exit_handler(struct intr_frame *f);
void practice_handler(struct intr_frame *f);
void filesize_handler(struct intr_frame *f);
void close_handler(struct intr_frame *f);
void tell_handler(struct intr_frame *f);
void seek_handler(struct intr_frame *f);
void halt_handler(struct intr_frame *f);
void create_handler(struct intr_frame *f);
void write_handler(struct intr_frame *f);
void read_handler(struct intr_frame *f);
void remove_handler(struct intr_frame *f);
static struct file *get_file_from_fd (int fd);

void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  sema_init(&file_sema,1);
}

static void
syscall_handler (struct intr_frame *f UNUSED)
{
  uint32_t* args = ((uint32_t*) f->esp);
    if(!is_valid_ptr(&args[0],sizeof (uint32_t))){
        exit_process(-1);
        NOT_REACHED();
    }
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
    case SYS_EXEC:
       exec_handler (f);
       break;
    case SYS_WAIT:
        wait_handler (f);
        break;
    case SYS_HALT:
      halt_handler (f);
      break;
    case SYS_CREATE:
        create_handler(f);
        break;
    case SYS_REMOVE:
        remove_handler(f);
        break;
    case SYS_WRITE:
        write_handler(f);
       break;
    case SYS_READ:
       read_handler(f);
        break;
    default:
      break;
    }
}

void halt_handler (struct intr_frame *f)
{
  uint32_t* args = ((uint32_t*) f->esp);
  shutdown_power_off (); // Defined in shutdown.c
}

void practice_handler (struct intr_frame *f)
{
  uint32_t* args = ((uint32_t*) f->esp);
  /* Make sure args[1] is valid */
  bool arg_valid = is_valid_ptr (&args[1], 4);
  /* Exit and free resources if not valid */
  if (!arg_valid) {
      exit_process(-1);
      NOT_REACHED();
  }
  f->eax = args[1] + 1;
}

void filesize_handler (struct intr_frame *f)
{
  uint32_t* args = ((uint32_t*) f->esp);
  /* Make sure args[1] is valid */
  bool arg_valid = is_valid_ptr (&args[1], 4);
  /* Exit and free resources if not valid */
  if (!arg_valid) {
      exit_process(-1);
      NOT_REACHED();
  }
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
        NOT_REACHED();
    }
}

void close_handler (struct intr_frame *f)
{
  uint32_t* args = ((uint32_t*) f->esp);
  /* Make sure args[1] is valid */
  bool arg_valid = is_valid_ptr (&args[1], 4);
  /* Exit and free resources if not valid */
  if (!arg_valid) {
      exit_process(-1);
      NOT_REACHED();
  }

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
      NOT_REACHED();
    }
}

void tell_handler (struct intr_frame *f)
{
  uint32_t* args = ((uint32_t*) f->esp);
  /* Make sure args[1] is valid */
  bool arg_valid = is_valid_ptr (&args[1], 4);
  /* Exit and free resources if not valid */
  if (!arg_valid) {
      exit_process(-1);
      NOT_REACHED();
  }

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
      NOT_REACHED();
    }
}

void seek_handler (struct intr_frame *f)
{
  uint32_t* args = ((uint32_t*) f->esp);
  /* Make sure args[1] & args[2] are valid */
  bool arg_valid = is_valid_ptr (&args[1], 8);
  /* Exit and free resources if not valid */
  if (!arg_valid) {
      exit_process(-1);
      NOT_REACHED();
  }

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
        NOT_REACHED();
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
        NOT_REACHED();
      }
    }
  else
    {
      exit_process (-1);
      NOT_REACHED();
    }
}

/* Handle exec Syscall */
void exec_handler(struct intr_frame *f){
  uint32_t* args = ((uint32_t*) f->esp);

  if(!is_valid_str (args[1])){
    exit_process(-1);
    NOT_REACHED();
  }
  char* command_line = args[1];
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
          f->eax=tid;
          return;
        }
    }
  /* Child thread not found */
  f->eax = -1;
}

/* Handle wait Syscall */
void wait_handler(struct intr_frame *f){
    uint32_t* args = ((uint32_t*) f->esp);
    if(!is_valid_ptr (&args[1],sizeof (tid_t))){
        exit_process(-1);
        NOT_REACHED();
    }
    tid_t child_tid = args[1];

  f->eax = process_wait (child_tid);
}

/* Handle exit Syscall */
void exit_handler(struct intr_frame *f){
  uint32_t* args = ((uint32_t*) f->esp);
  if(!is_valid_ptr (&args[1],sizeof (int))){
        exit_process(-1);
        NOT_REACHED();
  }
  int exit_code = args[1];
  f->eax = exit_code;
  struct thread *current_thread = thread_current ();
  current_thread->its_child->exit_code = exit_code;
  printf ("%s: exit(%d)\n", &thread_current ()->name, exit_code);
  thread_exit ();
}

/* Exit and kill current process */
void exit_process (int exit_code)
{
  struct thread *current_thread = thread_current ();
  current_thread->its_child->exit_code = exit_code;
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

void create_handler(struct intr_frame *f){
    uint32_t* args = ((uint32_t*) f->esp);
    if (!is_valid_str(args[1])){
        exit_process(-1);
        NOT_REACHED();
    }
    if (!is_valid_ptr(&args[2], 4)){
        exit_process(-1);
        NOT_REACHED();
    }
    const char* file_name = (const char*) args[1];
    size_t size = (size_t) args[2];
    bool status = false;
    sema_down(&file_sema);
    status = filesys_create(file_name, size);
    sema_up(&file_sema);
    f->eax = status;
}

void remove_handler(struct intr_frame *f){
    uint32_t* args = ((uint32_t*) f->esp);
    if (!is_valid_str(args[1])){
        exit_process(-1);
        NOT_REACHED();
    }
    const char* file_name = (const char*) args[1];
    bool status = false;
    sema_down(&file_sema);
    status = filesys_remove(file_name);
    sema_up(&file_sema);
    f->eax = status;
}

void write_handler(struct intr_frame *f){
    uint32_t* args = ((uint32_t*) f->esp);
    if (!is_valid_ptr(&args[1], 12)){
        exit_process(-1);
        NOT_REACHED();
    }
    int file_des = (int) args[1];
    const void* buffer = (const void*) args[2];
    size_t buffer_size = (size_t) args[3];
    struct file* fds;
    int write_size;
    void* buffer_copy = buffer;
    size_t copy_buffer_size = buffer_size;
    sema_down(&file_sema);
    switch (file_des){
        case STDIN_FILENO:
            write_size = -1;
             exit_process(-1);
            break;
        case STDOUT_FILENO:
            putbuf(buffer, buffer_size);
            write_size = buffer_size;
            break;
        default:
            fds = get_file_from_fd(file_des);
            if (fds != NULL){
                write_size = file_write(fds, buffer, buffer_size);
            }

            break;
    }
    sema_up(&file_sema);
    f->eax = write_size;
}



void read_handler(struct intr_frame *f){
    uint32_t* args = ((uint32_t*) f->esp);
    if (!is_valid_ptr(&args[1], 12)){
        exit_process(-1);
        NOT_REACHED();
    }
    int file_des = (int) args[1];
    void* buffer = (void*) args[2];
    size_t buffer_size = (size_t) args[3];
    struct file* fds;
    int read_size;
    void* buffer_copy = buffer;
    size_t copy_buffer_size = buffer_size;
    sema_down(&file_sema);
    uint8_t c;
    unsigned counter = buffer_size;
    uint8_t *buf = buffer;
    switch (file_des){
        case STDIN_FILENO:
            while (counter > 1 && (c = input_getc()) != 0)
            {
                *buf = c;
                buffer = buffer + 1;
                counter = counter - 1;
            }
            *buf = 0;
            read_size = buffer_size - counter;
            break;
        case STDOUT_FILENO:
            read_size = -1;
            // exit_process(-1);
            break;
        default:
            fds = get_file_from_fd(file_des);
            if (fds != NULL){
                read_size = file_read(fds, buffer, buffer_size);
            }
            break;

    }
    sema_up(&file_sema);
    f->eax = read_size;
}
