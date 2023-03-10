#include "filesys/filesys.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "filesys/directory.h"
#include "threads/thread.h"

/* Partition that contains the file system. */
struct block *fs_device;

static void do_format (void);

/* Initializes the file system module.
   If FORMAT is true, reformats the file system. */
void
filesys_init (bool format)
{
  fs_device = block_get_role (BLOCK_FILESYS);
  if (fs_device == NULL)
    PANIC ("No file system device found, can't initialize file system.");

  inode_init ();
  free_map_init ();

  if (format)
    do_format ();

  free_map_open ();
}

/* Shuts down the file system module, writing any unwritten data
   to disk. */
void
filesys_done (void)
{
  free_map_close ();
  inode_write_dirty_to_disk ();
}

/* Creates a file named NAME with the given INITIAL_SIZE.
   Returns true if successful, false otherwise.
   Fails if a file named NAME already exists,
   or if internal memory allocation fails. */
bool
filesys_create (const char *name, off_t initial_size)
{
  block_sector_t inode_sector = 0;
    struct dir* dir = dir_open_root();
    char dir_absolute[128];
    if (name[0] != '/'){
        strlcpy(dir_absolute , thread_current()->cwd,sizeof(dir_absolute)+1 );
        strlcat(dir_absolute, "/",sizeof (dir_absolute)+1 );
        strlcat(dir_absolute, name,sizeof (dir_absolute)+1 );
    }else{
        strlcpy(dir_absolute , name,sizeof(dir_absolute)+1 );
    }

    if(strcmp(dir_absolute,"//")==0){
        dir_close (dir);
        return false;
    }
  uint32_t group_idx = find_preferred_group (inode_sector);
  bool success = (dir != NULL
                  && group_free_map_allocate (group_idx, 1, &inode_sector)
                  && inode_create (inode_sector, initial_size)
                  );
  char final_name[NAME_MAX+1];
  parse(dir, dir_absolute,final_name);
    success = success && dir_add (dir, final_name, inode_sector);
  if (!success && inode_sector != 0)
    free_map_release (inode_sector, 1);
  dir_close (dir);

  return success;
}

/* Opens the file with the given NAME.
   Returns the new file if successful or a null pointer
   otherwise.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
struct file *
filesys_open (const char *name)
{
  struct dir *dir = dir_open_root ();
  char dir_absolute[128];
  if (name[0] != '/'){
      strlcpy(dir_absolute , thread_current()->cwd,sizeof(dir_absolute)+1 );
      strlcat(dir_absolute, "/",sizeof (dir_absolute)+1 );
      strlcat(dir_absolute, name,sizeof (dir_absolute)+1 );
  }else{
      strlcpy(dir_absolute , name,sizeof(dir_absolute)+1 );
  }

  if(strcmp(dir_absolute,"//")==0){
      dir_close (dir);
      return false;
  }
  struct inode *inode = NULL;
  char final_name[NAME_MAX + 1];
  parse(dir, dir_absolute,final_name);
  if (dir != NULL)
    dir_lookup (dir, final_name, &inode);
  dir_close (dir);

  return file_open (inode);
}

/* Deletes the file named NAME.
   Returns true if successful, false on failure.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
bool
filesys_remove (const char *name)
{
  struct dir *dir = dir_open_root ();
  bool success = dir != NULL && dir_remove (dir, name);
  dir_close (dir);

  return success;
}

/* Formats the file system. */
static void
do_format (void)
{
  printf ("Formatting file system...");
  free_map_create ();
  if (!dir_create (ROOT_DIR_SECTOR, 16))
    PANIC ("root directory creation failed");
  free_map_close ();
  printf ("done.\n");
}
