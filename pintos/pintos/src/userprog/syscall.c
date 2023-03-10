#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "userprog/pagedir.h"
#include "devices/shutdown.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "devices/block.h"

#include "filesys/directory.h"
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
void open_handler(struct intr_frame *f);
void isdir_handler(struct intr_frame *f);
void mkdir_handler(struct intr_frame *f);
void chdir_handler(struct intr_frame *f);
int get_free_fd ();
int get_free_fd_for_dir();
static struct file *get_file_from_fd (int fd);

void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED)
{
  uint32_t* args = ((uint32_t*) f->esp);
    if (!is_valid_ptr (&args[0], sizeof (uint32_t)))
      {
        exit_process (-1);
        NOT_REACHED ();
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
    case SYS_OPEN:
      open_handler(f);
      break;
    case SYS_CHDIR:
        chdir_handler(f);
        break;
    case SYS_MKDIR:
        mkdir_handler(f);
        break;
    case SYS_ISDIR:
        isdir_handler(f);
        break;
      break;
    case SYS_BLOCK_READ_COUNT:
      block_read_count_handler (f);
      break;
    case SYS_BLOCK_WRITE_COUNT:
      block_write_count_handler (f);
      break;
    default:
      break;
    }
}

void isdir_handler(struct intr_frame *f){
    uint32_t* args = ((uint32_t*) f->esp);
    if (!is_valid_str (args[1]))
    {
        exit_process (-1);
        NOT_REACHED ();
    }
    if (!is_valid_ptr (&args[2], 4))
    {
        exit_process (-1);
        NOT_REACHED ();
    }
    int fd = (int) args[1];
    bool status = false;
    status = isdir(fd);
    f->eax = status;
}

void chdir_handler(struct intr_frame *f){
    uint32_t* args = ((uint32_t*) f->esp);
    if (!is_valid_str (args[1]))
    {
        exit_process (-1);
        NOT_REACHED ();
    }
    if (!is_valid_ptr (&args[2], 4))
    {
        exit_process (-1);
        NOT_REACHED ();
    }
    const char* dir_name = (const char*) args[1];
    bool status = false;
    status = chdir(dir_name);
    f->eax = status;
}
void mkdir_handler(struct intr_frame *f){
    uint32_t* args = ((uint32_t*) f->esp);
    if (!is_valid_str (args[1]))
    {
        exit_process (-1);
        NOT_REACHED ();
    }
    const char* dir_name = (const char*) args[1];
    bool status = false;
    status = mkdir(dir_name);
    f->eax = status;
}

void
halt_handler (struct intr_frame *f)
{
  uint32_t* args = ((uint32_t*) f->esp);
  shutdown_power_off (); // Defined in shutdown.c
}

void
practice_handler (struct intr_frame *f)
{
  uint32_t* args = ((uint32_t*) f->esp);
  /* Make sure args[1] is valid */
  bool arg_valid = is_valid_ptr (&args[1], 4);
  /* Exit and free resources if not valid */
  if (!arg_valid) 
    {
      exit_process (-1);
      NOT_REACHED ();
    }
  f->eax = args[1] + 1;
}

void
filesize_handler (struct intr_frame *f)
{
  uint32_t* args = ((uint32_t*) f->esp);
  /* Make sure args[1] is valid */
  bool arg_valid = is_valid_ptr (&args[1], 4);
  /* Exit and free resources if not valid */
  if (!arg_valid) 
    {
      exit_process (-1);
      NOT_REACHED ();
    }
  struct file *file = get_file_from_fd (args[1]);
  if (file != NULL)
    {
      f->eax = file_length (file);
    }
  else
    {
      exit_process (-1);
      NOT_REACHED ();
    }
}

void
close_handler (struct intr_frame *f)
{
  uint32_t* args = ((uint32_t*) f->esp);
  /* Make sure args[1] is valid */
  bool arg_valid = is_valid_ptr (&args[1], 4);
  /* Exit and free resources if not valid */
  if (!arg_valid)
    {
      exit_process (-1);
      NOT_REACHED ();
    }

  struct file *file = get_file_from_fd (args[1]);
  if (file != NULL)
    {
      file_close (file);

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
      NOT_REACHED ();
    }
}

void
tell_handler (struct intr_frame *f)
{
  uint32_t* args = ((uint32_t*) f->esp);
  /* Make sure args[1] is valid */
  bool arg_valid = is_valid_ptr (&args[1], 4);
  /* Exit and free resources if not valid */
  if (!arg_valid) 
    {
      exit_process (-1);
      NOT_REACHED ();
    }

  struct file *file = get_file_from_fd (args[1]);
  if (file != NULL)
    {
      f->eax = file_tell (file);
    }
  else
    {
      exit_process (-1);
      NOT_REACHED ();
    }
}

void
seek_handler (struct intr_frame *f)
{
  uint32_t* args = ((uint32_t*) f->esp);
  /* Make sure args[1] & args[2] are valid */
  bool arg_valid = is_valid_ptr (&args[1], 8);
  /* Exit and free resources if not valid */
  if (!arg_valid)
    {
      exit_process (-1);
      NOT_REACHED ();
    }

  struct file *file = get_file_from_fd (args[1]);
  if (file != NULL)
    {
      uint32_t f_size = file_length (file);
      int file_pos = (int) args[2];
      if (file_pos < 0)
        {
          exit_process (-1);
          NOT_REACHED ();
        }
      else
        {
          file_seek (file, args[2]);
        }
    }
  else
    {
      exit_process (-1);
      NOT_REACHED ();
    }
}

/* Handle exec Syscall */
void
exec_handler(struct intr_frame *f){
  uint32_t* args = ((uint32_t*) f->esp);

  /* Validate char *pointer first to ensure all four bytes are valid */
  if (!is_valid_ptr (&args[1], 4))
   {
      exit_process (-1);
      NOT_REACHED ();
   }

  /* Now validate the string to ensure its validity */
  if(!is_valid_str (args[1]))
    {
      exit_process (-1);
      NOT_REACHED ();
    }
  char* command_line = args[1];
  tid_t tid = process_execute (command_line);

  if (tid == TID_ERROR)
    {
      /* Process execution is failed */
      f->eax = -1;
      return;
    }

  struct thread *current_thread = thread_current ();
  struct list_elem *e;
  /* Trying to find Child elem with thread ID = tid */
  for (e = list_begin (&current_thread->children); e != list_end (&current_thread->children); 
       e = list_next (e))
    {
      struct child *child = list_entry (e, struct child, child_elem);
      if (child->tid == tid)
        {
           sema_down (&child->load_sem);
           if (!child->loaded_status)
             {
              /* Child loading was failed */
              f->eax = -1;
              return;
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
void
wait_handler(struct intr_frame *f)
{
  uint32_t* args = ((uint32_t*) f->esp);
  if(!is_valid_ptr (&args[1],sizeof (tid_t)))
    {
      exit_process (-1);
      NOT_REACHED ();
    }
  tid_t child_tid = args[1];

  f->eax = process_wait (child_tid);
}

/* Handle exit Syscall */
void
exit_handler(struct intr_frame *f)
{
  uint32_t* args = ((uint32_t*) f->esp);
  if(!is_valid_ptr (&args[1],sizeof (int)))
    {
      exit_process (-1);
      NOT_REACHED ();
    }
  int exit_code = args[1];
  f->eax = exit_code;
  struct thread *current_thread = thread_current ();
  current_thread->its_child->exit_code = exit_code;
  printf ("%s: exit(%d)\n", &thread_current ()->name, exit_code);
  thread_exit ();
}

/* Exit and kill current process */
void
exit_process (int exit_code)
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

static struct file *
get_file_from_dir_fd (int fd)
{
    struct thread *current = thread_current ();
    struct list_elem *e;
    for (e = list_begin (&current->open_dirs); e != list_end (&current->open_dirs);
         e = list_next (e))
    {
        struct open_dir *od = list_entry (e, struct open_dir, dir_elem);
        if (od->fd == fd)
        {
            return od->this_dir;
        }
    }

    /* When reach here, fd cannot be found */
    return NULL;
}


void
create_handler (struct intr_frame *f)
{
    uint32_t* args = ((uint32_t*) f->esp);
    if (!is_valid_str (args[1]))
      {
        exit_process (-1);
        NOT_REACHED ();
      }
    if (!is_valid_ptr (&args[2], 4))
      {
        exit_process (-1);
        NOT_REACHED ();
      }
    const char* file_name = (const char*) args[1];
    size_t size = (size_t) args[2];
    bool status = false;
    status = filesys_create (file_name, size);
    f->eax = status;
}

void
remove_handler (struct intr_frame *f)
{
    uint32_t* args = ((uint32_t*) f->esp);
    if (!is_valid_str (args[1]))
    {
      exit_process (-1);
      NOT_REACHED ();
    }
    const char* file_name = (const char*) args[1];
    bool status = false;
    status = filesys_remove (file_name);
    f->eax = status;
}

void
write_handler(struct intr_frame *f)
{
    uint32_t* args = ((uint32_t*) f->esp);
    if (!is_valid_ptr (&args[1], 12)){
      exit_process (-1);
      NOT_REACHED ();
    }
    int file_des = (int) args[1];
    const void* buffer = (const void*) args[2];
    size_t buffer_size = (size_t) args[3];

    /* Must validate the buffer itsself */
    if (!is_valid_ptr (buffer, buffer_size))
     {
       exit_process (-1);
       NOT_REACHED ();
     }

    struct file* fds;
    int write_size;
    void* buffer_copy = buffer;
    size_t copy_buffer_size = buffer_size;
    switch (file_des)
      {
        case STDIN_FILENO:
            write_size = -1;
            break;
        case STDOUT_FILENO:
            putbuf(buffer, buffer_size);
            write_size = buffer_size;
            break;
        default:
            if(file_des%2==0&&file_des>2)
                fds = get_file_from_dir_fd(file_des);
            else
                fds = get_file_from_fd(file_des);
            if (fds != NULL)
              {
                if(file_des%2!=0&&file_des>2)
                    write_size = file_write(fds, buffer, buffer_size);
                else{
                    write_size=-1;
                    break;
                }
              }

            break;
      }
    f->eax = write_size;
}



void
read_handler(struct intr_frame *f)
{
    uint32_t* args = ((uint32_t*) f->esp);
    if (!is_valid_ptr (&args[1], 12))
      {
        exit_process (-1);
        NOT_REACHED ();
      }
    int file_des = (int) args[1];
    void* buffer = (void*) args[2];
    size_t buffer_size = (size_t) args[3];

    /* Must validate the buffer itsself */
    if (!is_valid_ptr (buffer, buffer_size))
     {
       exit_process (-1);
       NOT_REACHED ();
     }
    struct file* fds;
    int read_size;
    void* buffer_copy = buffer;
    size_t copy_buffer_size = buffer_size;
    uint8_t c;
    unsigned counter = buffer_size;
    uint8_t *buf = buffer;
    switch (file_des)
      {
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
            break;
        default:
            fds = get_file_from_fd(file_des);
            if (fds != NULL)
              {
                if(file_des%2!=0)
                    read_size = file_read(fds, buffer, buffer_size);
                else{
                    read_size=-1;
                    break;
                }

              }
            break;

      }
    f->eax = read_size;
}

void
open_handler (struct intr_frame *f)
{
  uint32_t* args = ((uint32_t*) f->esp);
  if (!is_valid_str (args[1]))
    {
      exit_process (-1);
      NOT_REACHED ();
    }
  struct file *curr_file;
  struct dir_entry e;
  struct dir* curr_dir;
  struct dir* dir = dir_open_root();
  const char* addr = (char*)args[1];
    char dir_absolute[128];
  if (addr[0] != '/'){
      strlcpy(dir_absolute , thread_current()->cwd,sizeof(dir_absolute)+1 );
      strlcat(dir_absolute, "/",sizeof (dir_absolute)+1 );
      strlcat(dir_absolute, addr,sizeof (dir_absolute)+1 );
  }else{
      strlcpy(dir_absolute , addr,sizeof(dir_absolute)+1 );
  }
  char name[NAME_MAX+1];
  parse(dir, dir_absolute,name);
  bool is_dir;
  if(strcmp(dir_absolute,"/")==0){
      is_dir=true;
      e.is_dir=true;
      e.inode_sector = ROOT_DIR_SECTOR;
  }else {
      is_dir = get_dir_entry(dir, name, &e, NULL);
  }
  if (is_dir && e.is_dir){
      curr_dir = dir_open(inode_open(e.inode_sector));
      if (curr_dir == NULL)
      {
          f->eax = -1;
      }
      else
      {
          struct thread *current = thread_current ();
          struct open_dir *od = (struct open_dir *) malloc (sizeof (struct open_dir));
          od->this_dir = curr_dir;
          od->fd = get_free_fd_for_dir();
          list_push_back (&current->open_dirs, &od->dir_elem);
          f->eax = od->fd;
      }
  }else{
      curr_file = filesys_open (args[1]);
      if (curr_file == NULL)
      {
          f->eax = -1;
      }
      else
      {
          struct thread *current = thread_current ();
          struct open_file *of = (struct open_file *) malloc (sizeof (struct open_file));
          of->this_file = curr_file;
          of->fd = get_free_fd ();
          list_push_back (&current->open_files, &of->file_elem);
          f->eax = of->fd;
      }
  }

}

/* Returns a valid file descriptor not already allocated for this thread */
int get_free_fd () 
{
  struct thread *current = thread_current ();
  if (list_empty (&current->open_files))
    return 3; // 0, 1, and 2 are already allocated to STDIN, STDOUT, and STDERR
  
  struct list_elem *e;
  e = list_back (&current->open_files); // Should ensure list is not empty
  struct open_file *of = list_entry (e, struct open_file, file_elem);
  return (of->fd + 2);
}

int get_free_fd_for_dir()
{
    struct thread *current = thread_current ();
    if (list_empty (&current->open_dirs))
        return 4; // 0, 1, and 2 are already allocated to STDIN, STDOUT, and STDERR

    struct list_elem *e;
    e = list_back (&current->open_dirs); // Should ensure list is not empty
    struct open_dir *od = list_entry (e, struct open_dir, dir_elem);
    return (od->fd + 2);
}
void
block_read_count_handler (struct intr_frame *f)
{
    f->eax = (unsigned) fs_device->read_cnt;
}

void
block_write_count_handler (struct intr_frame *f)
{
    f->eax = (unsigned) fs_device->write_cnt;
}