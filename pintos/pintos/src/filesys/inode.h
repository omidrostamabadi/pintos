#ifndef FILESYS_INODE_H
#define FILESYS_INODE_H

#include <stdbool.h>
#include <bitmap.h>
#include "filesys/off_t.h"
#include "devices/block.h"
#include <list.h>

/* On-disk inode.
   Must be exactly BLOCK_SECTOR_SIZE bytes long. */
struct inode_disk
{
    block_sector_t start;               /* First data sector. */
    off_t length;                       /* File size in bytes. */
    unsigned magic;                     /* Magic number. */
    block_sector_t direct_ptr[123];
    block_sector_t indirect_ptr;
    block_sector_t dbl_indirect_ptr;
};

/* In-memory inode. */
struct inode
{
    struct list_elem elem;              /* Element in inode list. */
    block_sector_t sector;              /* Sector number of disk location. */
    int open_cnt;                       /* Number of openers. */
    bool removed;                       /* True if deleted, false otherwise. */
    int deny_write_cnt;                 /* 0: writes ok, >0: deny writes. */
    struct inode_disk data;             /* Inode content. */
};

struct bitmap;

void inode_init (void);
bool inode_create (block_sector_t, off_t);
struct inode *inode_open (block_sector_t);
struct inode *inode_reopen (struct inode *);
block_sector_t inode_get_inumber (const struct inode *);
void inode_close (struct inode *);
void inode_remove (struct inode *);
off_t inode_read_at (struct inode *, void *, off_t size, off_t offset);
off_t inode_write_at (struct inode *, const void *, off_t size, off_t offset);
void inode_deny_write (struct inode *);
void inode_allow_write (struct inode *);
off_t inode_length (const struct inode *);

static struct cache_entry *check_busy (int);
static int clock_evict_block ();
static void fetch_new_block (int, block_sector_t);

static uint32_t GROUP_SIZE = 512;
static uint32_t GROUP_COUNT = 8;
struct bitmap **group_bitmaps;
static void
normal_read (block_sector_t sector, void *buffer, int sector_ofs, int chunk_size);
static void
normal_write (block_sector_t sector, void *buffer, int sector_ofs, int chunk_size);
bool
extend_inode(struct inode* data_inode,off_t offset,size_t size);

uint32_t find_preferred_group(block_sector_t sector);
#endif /* filesys/inode.h */
