#ifndef FILESYS_DIRECTORY_H
#define FILESYS_DIRECTORY_H

#include <stdbool.h>
#include <stddef.h>
#include "devices/block.h"
#include "filesys/off_t.h"
/* Maximum length of a file name component.
   This is the traditional UNIX maximum length.
   After directories are implemented, this maximum length may be
   retained, but much longer full path names must be allowed. */
#define NAME_MAX 30

struct inode;
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

/* Opening and closing directories. */
bool dir_create (block_sector_t sector, size_t entry_cnt);
struct dir *dir_open (struct inode *);
struct dir *dir_open_root (void);
struct dir *dir_reopen (struct dir *);
void dir_close (struct dir *);
struct inode *dir_get_inode (struct dir *);

/* Reading and writing. */
bool dir_lookup (const struct dir *, const char *name, struct inode **);
bool dir_add (struct dir *, const char *name, block_sector_t);
bool dir_remove (struct dir *, const char *name);
bool dir_readdir (struct dir *, char name[NAME_MAX + 1]);
bool mkdir(const char* dir_name);
bool isdir(int fd);
bool chdir(const char* dir_name);
static int get_next_part(char part[NAME_MAX + 1], const char** srcp);
const char* parse(struct dir** dir, char* input);
struct dir* find_working_directory(const struct dir* cur_dir, const char* name);
bool
search_dir(const struct dir *dir, const char *name,
           struct inode **inode);
bool
add_to_dir(struct dir *dir, const char *name, block_sector_t inode_sector);
bool
get_dir_entry(const struct dir *dir, const char *name,
              struct dir_entry *ep, off_t *ofsp);
#endif /* filesys/directory.h */
