#include "filesys/directory.h"
#include <stdio.h>
#include <string.h>
#include <list.h>
#include "filesys/filesys.h"
#include "filesys/inode.h"
#include "threads/malloc.h"
#include "threads/thread.h"

/* Creates a directory with space for ENTRY_CNT entries in the
   given SECTOR.  Returns true if successful, false on failure. */
bool
dir_create (block_sector_t sector, size_t entry_cnt)
{
  return inode_create (sector, entry_cnt * sizeof (struct dir_entry));
}

/* Opens and returns the directory for the given INODE, of which
   it takes ownership.  Returns a null pointer on failure. */
struct dir *
dir_open (struct inode *inode)
{
  struct dir *dir = calloc (1, sizeof *dir);
  if (inode != NULL && dir != NULL)
    {
      dir->inode = inode;
      dir->pos = 0;
      return dir;
    }
  else
    {
      inode_close (inode);
      free (dir);
      return NULL;
    }
}

/* Opens the root directory and returns a directory for it.
   Return true if successful, false on failure. */
struct dir *
dir_open_root (void)
{
  return dir_open (inode_open (ROOT_DIR_SECTOR));
}

/* Opens and returns a new directory for the same inode as DIR.
   Returns a null pointer on failure. */
struct dir *
dir_reopen (struct dir *dir)
{
  return dir_open (inode_reopen (dir->inode));
}

/* Destroys DIR and frees associated resources. */
void
dir_close (struct dir *dir)
{
  if (dir != NULL)
    {
      inode_close (dir->inode);
      free (dir);
    }
}

/* Returns the inode encapsulated by DIR. */
struct inode *
dir_get_inode (struct dir *dir)
{
  return dir->inode;
}

/* Searches DIR for a file with the given NAME.
   If successful, returns true, sets *EP to the directory entry
   if EP is non-null, and sets *OFSP to the byte offset of the
   directory entry if OFSP is non-null.
   otherwise, returns false and ignores EP and OFSP. */
static bool
lookup (const struct dir *dir, const char *name,
        struct dir_entry *ep, off_t *ofsp)
{
  struct dir_entry e;
  size_t ofs;

  ASSERT (dir != NULL);
  ASSERT (name != NULL);

  for (ofs = 0; inode_read_at (dir->inode, &e, sizeof e, ofs) == sizeof e;
       ofs += sizeof e)
    if (e.in_use && !strcmp (name, e.name))
      {
        if (ep != NULL)
          *ep = e;
        if (ofsp != NULL)
          *ofsp = ofs;
        return true;
      }
  return false;
}

void
list_files_in_root_dir ()
{
  struct dir *dir = dir_open_root();
  struct dir_entry e;
  size_t ofs;
  for (ofs = 0; inode_read_at (dir->inode, &e, sizeof e, ofs) == sizeof e;
       ofs += sizeof e)
    if (e.in_use)
      {
        printf("NAME: %s SECTOR: %u\n", e.name, e.inode_sector);
      }
}

/* Searches DIR for a file with the given NAME
   and returns true if one exists, false otherwise.
   On success, sets *INODE to an inode for the file, otherwise to
   a null pointer.  The caller must close *INODE. */
bool
dir_lookup (const struct dir *dir, const char *name,
            struct inode **inode)
{
  struct dir_entry e;

  ASSERT (dir != NULL);
  ASSERT (name != NULL);

  if (lookup (dir, name, &e, NULL))
    *inode = inode_open (e.inode_sector);
  else
    *inode = NULL;

  return *inode != NULL;
}

/* Adds a file named NAME to DIR, which must not already contain a
   file by that name.  The file's inode is in sector
   INODE_SECTOR.
   Returns true if successful, false on failure.
   Fails if NAME is invalid (i.e. too long) or a disk or memory
   error occurs. */
bool
dir_add (struct dir *dir, const char *name, block_sector_t inode_sector)
{
  struct dir_entry e;
  off_t ofs;
  bool success = false;

  ASSERT (dir != NULL);
  ASSERT (name != NULL);

  /* Check NAME for validity. */
  if (*name == '\0' || strlen (name) > NAME_MAX)
    return false;
  const char* name_copy = malloc(strlen(name) * sizeof(char) + 1);
  strlcpy(name_copy, name, strlen(name) * sizeof(char) + 1);
  /* Check that NAME is not in use. */
  if (lookup (dir, name, NULL, NULL))
    goto done;

  /* Set OFS to offset of free slot.
     If there are no free slots, then it will be set to the
     current end-of-file.

     inode_read_at() will only return a short read at end of file.
     Otherwise, we'd need to verify that we didn't get a short
     read due to something intermittent such as low memory. */
  for (ofs = 0; inode_read_at (dir->inode, &e, sizeof e, ofs) == sizeof e;
       ofs += sizeof e)
    if (!e.in_use)
      break;

  /* Write slot. */
  e.in_use = true;
  e.is_dir = false;
  strlcpy (e.name, name_copy, sizeof e.name);
  e.inode_sector = inode_sector;
  success = inode_write_at (dir->inode, &e, sizeof e, ofs) == sizeof e;

 done:
  return success;
}

/* Removes any entry for NAME in DIR.
   Returns true if successful, false on failure,
   which occurs only if there is no file with the given NAME. */
bool
dir_remove (struct dir *dir, const char *name)
{
  struct dir_entry e;
  struct inode *inode = NULL;
  bool success = false;
  off_t ofs;

  ASSERT (dir != NULL);
  ASSERT (name != NULL);

  /* Find directory entry. */
  if (!lookup (dir, name, &e, &ofs))
    goto done;

  /* Open inode. */
  inode = inode_open (e.inode_sector);
  if (inode == NULL)
    goto done;

  /* Erase directory entry. */
  e.in_use = false;
  if (inode_write_at (dir->inode, &e, sizeof e, ofs) != sizeof e)
    goto done;

  /* Remove inode. */
  inode_remove (inode);
  success = true;

 done:
  inode_close (inode);
  return success;
}

/* Reads the next directory entry in DIR and stores the name in
   NAME.  Returns true if successful, false if the directory
   contains no more entries. */
bool
dir_readdir (struct dir *dir, char name[NAME_MAX + 1])
{
  struct dir_entry e;

  while (inode_read_at (dir->inode, &e, sizeof e, dir->pos) == sizeof e)
    {
      dir->pos += sizeof e;
      if (e.in_use)
        {
          strlcpy (name, e.name, NAME_MAX + 1);
          return true;
        }
    }
  return false;
}

bool isdir(int fd){
  return fd > 2 && (fd % 2) == 0;
}

bool chdir(const char* dir_name){
  struct dir *root_dir = dir_open_root ();
  char dir_absolute[128];
  if (dir_name[0] != '/'){
      strlcpy(dir_absolute , thread_current()->cwd,sizeof(dir_absolute)+1 );
      strlcat(dir_absolute, "/",sizeof (dir_absolute)+1 );
      strlcat(dir_absolute, dir_name,sizeof (dir_absolute)+1 );
  }else{
      strlcpy(dir_absolute , dir_name,sizeof(dir_absolute)+1 );
  }
  const char name_copy[128];
  strlcpy(name_copy, dir_absolute,sizeof (name_copy)+1);
  char name[NAME_MAX + 1];
  parse(root_dir, dir_absolute,name);
  if(lookup(root_dir, name, NULL, NULL)) {
      thread_current()->cwd = dir_name;
      return true;
  }else{
      return false;
  }
}

bool mkdir(const char* dir_name){
  if (strcmp(dir_name, "") == 0){
      return false;
  }
  char dir_absolute[128];
  if (dir_name[0] != '/'){
      strlcpy(dir_absolute , thread_current()->cwd,sizeof(dir_absolute)+1 );
      strlcat(dir_absolute, "/",sizeof (dir_absolute)+1 );
      strlcat(dir_absolute, dir_name,sizeof (dir_absolute)+1 );
  }else{
      strlcpy(dir_absolute , dir_name,sizeof(dir_absolute)+1 );
  }
  block_sector_t inode_sector = 0;
  struct dir *root_dir = dir_open_root ();
  uint32_t group_idx = 0;
  bool success = (root_dir != NULL
          && group_free_map_allocate (group_idx, 1, &inode_sector)
          && dir_create (inode_sector, 16));
  struct dir_entry e;
  char symbol[NAME_MAX + 1];
  parse(root_dir, dir_absolute,symbol);
  success = success && add_to_dir(root_dir, (const char*)symbol, inode_sector);
  if (!success && inode_sector != 0)
      free_map_release (inode_sector, 1);
  struct dir* new_dir = dir_open(inode_open(inode_sector));
  add_to_dir(new_dir, ".", inode_sector);
  add_to_dir(new_dir, "..", root_dir->inode->sector);
  dir_close (new_dir);
  dir_close (root_dir);
  return success;
}
struct dir* find_working_directory(const struct dir* cur_dir, const char* name){
  struct inode* cur_inode = NULL;
  if(!search_dir(cur_dir, name, &cur_inode)){
      return NULL;
  }
  return dir_open(cur_inode);
}

void parse(struct dir *parent_dir, char* input,char symbol[NAME_MAX + 1]){
  struct dir *gp_dir;
  while (get_next_part(symbol, &input) == 1){
      gp_dir = parent_dir;
      parent_dir = find_working_directory(gp_dir, symbol);
  }
  parent_dir = gp_dir;
}



/* Extracts a file name part from *SRCP into PART, and updates *SRCP so that the
next call will return the next file name part. Returns 1 if successful, 0 at
end of string, -1 for a too-long file name part. */
static int get_next_part(char part[NAME_MAX + 1], const char** srcp) {
  const char* src = *srcp;
  char* dst = part;
/* Skip leading slashes. If it's all slashes, we're done. */
  while (*src == '/')
      src++;
  if (*src == '\0')
      return 0;
/* Copy up to NAME_MAX character from SRC to DST. Add null terminator. */
  while (*src != '/' && *src != '\0') {
      if (dst < part + NAME_MAX)
          *dst++ = *src;
      else
          return -1;
      src++;
  }
  *dst = '\0';
/* Advance source pointer. */
  *srcp = src;
  return 1;
}

bool
search_dir(const struct dir *dir, const char *name,
  struct inode **inode)
{
  struct dir_entry e;

  ASSERT (dir != NULL);
  ASSERT (name != NULL);

  if (lookup (dir, name, &e, NULL))
      *inode = inode_open (e.inode_sector);
  else
      *inode = NULL;

  return *inode != NULL && e.is_dir;
}

bool
add_to_dir(struct dir *dir, const char *name, block_sector_t inode_sector)
{
  struct dir_entry e;
  off_t ofs;
  bool success = false;

  ASSERT (dir != NULL);
  ASSERT (name != NULL);

  const char* name_copy = malloc(strlen(name) * sizeof(char) + 1);
  strlcpy(name_copy, name, strlen(name) * sizeof(char) + 1);
  /* Check NAME for validity. */
  if (*name == '\0' || strlen (name) > NAME_MAX)
      return false;
  /* Check that NAME is not in use. */
  if (lookup (dir, name, NULL, NULL))
      goto done;

    /* Set OFS to offset of free slot.
       If there are no free slots, then it will be set to the
       current end-of-file.

       inode_read_at() will only return a short read at end of file.
       Otherwise, we'd need to verify that we didn't get a short
       read due to something intermittent such as low memory. */
  for (ofs = 0; inode_read_at (dir->inode, &e, sizeof e, ofs) == sizeof e;
    ofs += sizeof e)
    if (!e.in_use)
        break;

  /* Write slot. */
  e.in_use = true;
  e.is_dir = true;
  strlcpy (e.name, name_copy, sizeof e.name);
  e.inode_sector = inode_sector;
  success = inode_write_at (dir->inode, &e, sizeof e, ofs) == sizeof e;

  done:
    return success;
}

bool
get_dir_entry(const struct dir *dir, const char *name,
  struct dir_entry *ep, off_t *ofsp)
{
  struct dir_entry e;
  size_t ofs;

  ASSERT (dir != NULL);
  ASSERT (name != NULL);
  const char* name_copy = malloc(strlen(name) * sizeof(char) + 1);
  strlcpy(name_copy, name, strlen(name) * sizeof(char) + 1);
  for (ofs = 0; inode_read_at (dir->inode, &e, sizeof e, ofs) == sizeof e;
     ofs += sizeof e)
    if (e.in_use && !strcmp (name_copy, e.name))
    {
        if (ep != NULL)
            *ep = e;
        if (ofsp != NULL)
            *ofsp = ofs;
        return true;
    }
  return false;
}