#include "filesys/directory.h"
#include <stdio.h>
#include <string.h>
#include <list.h>
#include "filesys/filesys.h"
#include "filesys/inode.h"
#include "threads/malloc.h"
#include "threads/thread.h"
/* A directory. */
struct dir
  {
    struct inode *inode;                /* Backing store. */
    off_t pos;                          /* Current position. */
    struct lock* dir_lock;
  };

/* A single directory entry. */
struct dir_entry
  {
    block_sector_t inode_sector;        /* Sector number of header. */
    char name[NAME_MAX + 1];            /* Null terminated file name. */
    bool in_use;                        /* In use or free? */
    bool is_dir;
  };

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

//bool isdir(){
//
//}

bool chdir(const char* dir_name){
    thread_current()->cwd = dir_name;
    /// TODO: Handle special cases later
    return true;
}

bool mkdir(const char* dir_name){
//    if (dir_name[0] != '/'){
//        char* dir_absolute = malloc(sizeof(dir_name)/sizeof(dir_name[0]) +
//                sizeof(thread_current()->cwd)/sizeof((thread_current()->cwd)[0]) + 1);
//        memcpy(dir_absolute, thread_current()->cwd, sizeof(thread_current()->cwd));
//        memcpy(dir_absolute + sizeof(thread_current()->cwd), thread_current()->cwd, );
//    }
    if (strcmp(dir_name, "") == 0){
        return false;
    }
    block_sector_t inode_sector = 0;
    struct dir *dir = dir_open_root ();
    uint32_t group_idx = 0;
    bool success = (dir != NULL
                    && group_free_map_allocate (group_idx, 1, &inode_sector)
                    && dir_create (inode_sector, 16));
    struct dir_entry e;
    const char* name = parse(&dir, dir_name);
//    if (){
//        dir_close(dir);
//        return false;
//    }
    const char* name_copy = malloc(strlen(name) * sizeof(char) + 1);
    strlcpy(name_copy, name, strlen(name) * sizeof(char) + 1);
    success = success && add_to_dir(dir, name, inode_sector);
    if (!success && inode_sector != 0)
        free_map_release (inode_sector, 1);
    struct dir* new_dir = dir_open(inode_open(inode_sector));
//    lookup(dir, name_copy, &e, NULL);
//    e.is_dir = true;
    add_to_dir(new_dir, ".", inode_sector);
    add_to_dir(new_dir, "..", dir->inode->sector);
    dir_close (new_dir);
    dir_close (dir);
    return success;

//    block_sector_t sector;
//    thread_current()->cwd = dir_open_root();
//    if (!dir_create(sector, 16)){
//     return false;
//    }
////    const char* name = (const char*) parse(dir);
//    if (!dir_add(thread_current()->cwd, parse(dir), sector)){
//     return false;
//    }
//    return true;
}
struct dir* find_working_directory(const struct dir* cur_dir, const char* name){
    struct inode* cur_inode = NULL;
    if(!search_dir(cur_dir, name, &cur_inode)){
        return NULL;
    }
    return dir_open(cur_inode);
}

const char* parse(struct dir** dir, char* input){
    struct dir* parent_dir;
    char symbol[NAME_MAX + 1];
    while (get_next_part(symbol, &input) == 1){
        parent_dir = find_working_directory(*dir, symbol);
        if (parent_dir != NULL){
            dir_close(*dir);
            *dir = parent_dir;
        }
    }
    const char* final_symbol = symbol;
    return final_symbol;
}
//struct dir* readdir(){
//
//}


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
    e.is_dir = true;
    strlcpy (e.name, name_copy, sizeof e.name);
    e.inode_sector = inode_sector;
    success = inode_write_at (dir->inode, &e, sizeof e, ofs) == sizeof e;

    done:
    return success;
}