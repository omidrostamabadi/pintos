#include "filesys/inode.h"
#include <list.h>
#include <debug.h>
#include <round.h>
#include <string.h>
#include "filesys/filesys.h"
#include "filesys/free-map.h"
#include "threads/malloc.h"
#include "threads/synch.h"

/* Ignore caching system. Kept for comparison */
//#define CACHE_BYPASS

/* Identifies an inode. */
#define INODE_MAGIC 0x494e4f44

/* Number of cache elements */
#define CACHE_ELEMENTS 64

/* On-disk inode.
   Must be exactly BLOCK_SECTOR_SIZE bytes long. */
struct inode_disk
  {
    block_sector_t start;               /* First data sector. */
    off_t length;                       /* File size in bytes. */
    unsigned magic;                     /* Magic number. */
    uint32_t unused[125];               /* Not used. */
  };

/* Returns the number of sectors to allocate for an inode SIZE
   bytes long. */
static inline size_t
bytes_to_sectors (off_t size)
{
  return DIV_ROUND_UP (size, BLOCK_SECTOR_SIZE);
}

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

/* Data structures, global, static variables for buffer cache system */

int clock_hand = 0; /* Used in clock algorithm */

struct cache_entry cache_entries[CACHE_ELEMENTS]; /* Buffer cache */

struct lock cache_lock; /* Acquire this lock prior to any modification to cache */

struct list busy_sectors; /* List of sectors that should not be accessed by
  other threads in the system */  

/* Entries stored in cache */
struct cache_entry
  {
    struct inode *its_inode; /* Inode for this cached sector */
    int used; /* Is set anytime there's an access to this entry */
    int dirty; /* If set, memory is inconsistent with disk */
    block_sector_t sector; /* Number of the sector that's been cached */
    struct lock busy_lock;
    struct list_elem busy_elem;
    char in_mem_data[BLOCK_SECTOR_SIZE]; /* Actual data of the sector */
  };

/* Sectors that should not be accessed by others */
struct busy_sector
  {
    struct inode *its_file; /* Which file does this belong? */
    block_sector_t sector; /* Sector number */
    struct lock busy_lock; /* Used for synchronization. 
      Other threads wait on this lock when they find the sector busy */
    struct list_elem busy_elem; /* Put elements in busy_sectors list */
  };

/* Helper functions for buffer cache system */

/* Checks cache to see if the sector is cached.
    Returns the index for the cache element containing the sector if found.
    If cannot find the sector in cache, returns -1.
    */
static int
check_cache (block_sector_t sector, struct inode *inode)
{
  for (int i = 0; i < CACHE_ELEMENTS; i++)
    {
      if (cache_entries[i].sector == sector && cache_entries[i].its_inode == inode)
        {
          return i;
        }
    }
  return -1; /* Sector not found in cache */
}

/* Put the sector which is being operated on (write, read, migrate to/from disk)
    into the busy_sectors list. If others want to operate on this sector, they
    wait by acquiring the lock. 
    The caller must hold cache_lock so that
    the state of the cache stays consistent.
    */
static void
add_to_busy (int cache_index)
{
  /* Assure that caller holds cache_lock */
  ASSERT (lock_held_by_current_thread (&cache_lock));

  struct cache_entry *this_sector = check_busy (cache_index);
  if (this_sector != NULL)
    {
      lock_release (&cache_lock);
      lock_acquire (& (this_sector->busy_lock));
      lock_acquire (&cache_lock);
    }
  else
    {
      this_sector = & (cache_entries[cache_index]);
    }  
  list_push_back (&busy_sectors, & (this_sector->busy_elem) );
}

/* Choose a cache block to be evicted in order to make room for a new one.
    Works based on clock algorithm */
static int
clock_evict_block ()
{
  struct cache_entry tmp;
  while (1)
    {
      tmp = cache_entries[clock_hand];
      if (tmp.used == 1)
        {
          tmp.used--;
          clock_hand++;
          if (clock_hand >= CACHE_ELEMENTS)
            clock_hand = 0;
        }
      else
        {
          int to_be_evicted = clock_hand;
          clock_hand++;
          if (clock_hand >= CACHE_ELEMENTS)
            clock_hand = 0;
          add_to_busy (to_be_evicted);
          return to_be_evicted;
        }
    }
}

/* Replaces sector_index sector with cache_entry in cache_index position.
    Writes previous data to disk if it is dirty. */
static void 
fetch_new_block (int cache_index, block_sector_t sector_index, struct inode *inode)
{
  if (cache_entries[cache_index].dirty == 1)
    {
      block_write (fs_device, cache_entries[cache_index].sector, 
                    cache_entries[cache_index].in_mem_data);
    }
  block_read (fs_device, sector_index, cache_entries[cache_index].in_mem_data);
  cache_entries[cache_index].dirty = 0;
  cache_entries[cache_index].used = 1;
  cache_entries[cache_index].its_inode = inode;
  cache_entries[cache_index].sector = sector_index;
}

/* Returns the address of struct busy_sector in busy_sectors list.
    If sector is not busy, returns NULL. */
static struct busy_sector *
check_busy (cache_index)
{
  struct cache_entry tmp = cache_entries[cache_index];
  for (e = list_begin (&busy_sectors); e != list_end (&busy_sectors);
       e = list_next (e))
    {
      struct busy_sector *bs = list_entry (e, struct busy_sector, busy_elem);
      if (bs->sector == tmp.sector && bs->its_file == tmp.its_node)
        return bs;
    }
  return NULL;
}
  
/* Returns the block device sector that contains byte offset POS
   within INODE.
   Returns -1 if INODE does not contain data for a byte at offset
   POS. */
static block_sector_t
byte_to_sector (const struct inode *inode, off_t pos)
{
  ASSERT (inode != NULL);
  if (pos < inode->data.length)
    return inode->data.start + pos / BLOCK_SECTOR_SIZE;
  else
    return -1;
}

/* List of open inodes, so that opening a single inode twice
   returns the same `struct inode'. */
static struct list open_inodes;

/* Initializes the inode module. */
void
inode_init (void)
{
  list_init (&open_inodes);
}

/* Initializes an inode with LENGTH bytes of data and
   writes the new inode to sector SECTOR on the file system
   device.
   Returns true if successful.
   Returns false if memory or disk allocation fails. */
bool
inode_create (block_sector_t sector, off_t length)
{
  struct inode_disk *disk_inode = NULL;
  bool success = false;

  ASSERT (length >= 0);

  /* If this assertion fails, the inode structure is not exactly
     one sector in size, and you should fix that. */
  ASSERT (sizeof *disk_inode == BLOCK_SECTOR_SIZE);

  disk_inode = calloc (1, sizeof *disk_inode);
  if (disk_inode != NULL)
    {
      size_t sectors = bytes_to_sectors (length);
      disk_inode->length = length;
      disk_inode->magic = INODE_MAGIC;
      if (free_map_allocate (sectors, &disk_inode->start))
        {
          block_write (fs_device, sector, disk_inode);
          if (sectors > 0)
            {
              static char zeros[BLOCK_SECTOR_SIZE];
              size_t i;

              for (i = 0; i < sectors; i++)
                block_write (fs_device, disk_inode->start + i, zeros);
            }
          success = true;
        }
      free (disk_inode);
    }
  return success;
}

/* Reads an inode from SECTOR
   and returns a `struct inode' that contains it.
   Returns a null pointer if memory allocation fails. */
struct inode *
inode_open (block_sector_t sector)
{
  struct list_elem *e;
  struct inode *inode;

  /* Check whether this inode is already open. */
  for (e = list_begin (&open_inodes); e != list_end (&open_inodes);
       e = list_next (e))
    {
      inode = list_entry (e, struct inode, elem);
      if (inode->sector == sector)
        {
          inode_reopen (inode);
          return inode;
        }
    }

  /* Allocate memory. */
  inode = malloc (sizeof *inode);
  if (inode == NULL)
    return NULL;

  /* Initialize. */
  list_push_front (&open_inodes, &inode->elem);
  inode->sector = sector;
  inode->open_cnt = 1;
  inode->deny_write_cnt = 0;
  inode->removed = false;
  block_read (fs_device, inode->sector, &inode->data);
  return inode;
}

/* Reopens and returns INODE. */
struct inode *
inode_reopen (struct inode *inode)
{
  if (inode != NULL)
    inode->open_cnt++;
  return inode;
}

/* Returns INODE's inode number. */
block_sector_t
inode_get_inumber (const struct inode *inode)
{
  return inode->sector;
}

/* Closes INODE and writes it to disk.
   If this was the last reference to INODE, frees its memory.
   If INODE was also a removed inode, frees its blocks. */
void
inode_close (struct inode *inode)
{
  /* Ignore null pointer. */
  if (inode == NULL)
    return;

  /* Release resources if this was the last opener. */
  if (--inode->open_cnt == 0)
    {
      /* Remove from inode list and release lock. */
      list_remove (&inode->elem);

      /* Deallocate blocks if removed. */
      if (inode->removed)
        {
          free_map_release (inode->sector, 1);
          free_map_release (inode->data.start,
                            bytes_to_sectors (inode->data.length));
        }

      free (inode);
    }
}

/* Marks INODE to be deleted when it is closed by the last caller who
   has it open. */
void
inode_remove (struct inode *inode)
{
  ASSERT (inode != NULL);
  inode->removed = true;
}

/* Reads SIZE bytes from INODE into BUFFER, starting at position OFFSET.
   Returns the number of bytes actually read, which may be less
   than SIZE if an error occurs or end of file is reached. */
off_t
inode_read_at (struct inode *inode, void *buffer_, off_t size, off_t offset)
{
  uint8_t *buffer = buffer_;
  off_t bytes_read = 0;
  uint8_t *bounce = NULL;

  while (size > 0)
    {
      /* Disk sector to read, starting byte offset within sector. */
      block_sector_t sector_idx = byte_to_sector (inode, offset);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually copy out of this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;

      #ifndef CACHE_BYPASS
      /* Check the cache for this sector. If the sector is busy, wait 
          until it's free and add it to busy again. If the sector is not
          present in the cache, read it from the disk. After these several lines
          of code, cache_index points to a valid cache block for the sector. */
      lock_acquire (&cache_lock);
      int cache_index = check_cache (sector_idx, inode);
      if (cache_index != -1) /* Sector found in cache */
        {
          add_to_busy (cache_index);
          lock_release (&cache_lock);
        }
      else /* Could not find the sector in cache */
        {
          cache_index = clock_evict_block ();
          lock_release (&cache_lock);
          fetch_new_block (cache_index, sector_idx, inode);
        }

      memcpy (buffer + bytes_read, 
              cache_entries[cache_index].in_mem_data + sector_ofs, chunk_size);

      /* We are done with this sector. Remove it from busy list */
      list_remove (& (cache_entries[cache_index].busy_elem) );
      lock_release (& (cache_entries[cache_index].busy_lock) );
      #else

      if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
        {
          /* Read full sector directly into caller's buffer. */
          block_read (fs_device, sector_idx, buffer + bytes_read);
        }
      else
        {
          /* Read sector into bounce buffer, then partially copy
             into caller's buffer. */
          if (bounce == NULL)
            {
              bounce = malloc (BLOCK_SECTOR_SIZE);
              if (bounce == NULL)
                break;
            }
          block_read (fs_device, sector_idx, bounce);
          memcpy (buffer + bytes_read, bounce + sector_ofs, chunk_size);
        }
        #endif /* CACHE_BYPASS */

      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_read += chunk_size;
    }
  free (bounce);

  return bytes_read;
}

/* Writes SIZE bytes from BUFFER into INODE, starting at OFFSET.
   Returns the number of bytes actually written, which may be
   less than SIZE if end of file is reached or an error occurs.
   (Normally a write at end of file would extend the inode, but
   growth is not yet implemented.) */
off_t
inode_write_at (struct inode *inode, const void *buffer_, off_t size,
                off_t offset)
{
  const uint8_t *buffer = buffer_;
  off_t bytes_written = 0;
  uint8_t *bounce = NULL;

  if (inode->deny_write_cnt)
    return 0;

  while (size > 0)
    {
      /* Sector to write, starting byte offset within sector. */
      block_sector_t sector_idx = byte_to_sector (inode, offset);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually write into this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;

      #ifndef CACHE_BYPASS
      /* Check the cache for this sector. If the sector is busy, wait 
          until it's free and add it to busy again. If the sector is not
          present in the cache, read it from the disk. After these several lines
          of code, cache_index points to a valid cache block for the sector. */
      lock_acquire (&cache_lock);
      int cache_index = check_cache (sector_idx, inode);
      if (cache_index != -1) /* Sector found in cache */
        {
          add_to_busy (cache_index);
          lock_release (&cache_lock);
        }
      else /* Could not find the sector in cache */
        {
          cache_index = clock_evict_block ();
          lock_release (&cache_lock);
          fetch_new_block (cache_index, sector_idx, inode);
        }

      memcpy (cache_entries[cache_index].in_mem_data + sector_ofs, 
              buffer + bytes_written, chunk_size);

      /* We are done with this sector. Remove it from busy list */
      list_remove (& (cache_entries[cache_index].busy_elem) );
      lock_release (& (cache_entries[cache_index].busy_lock) );
      #else
      if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
        {
          /* Write full sector directly to disk. */
          block_write (fs_device, sector_idx, buffer + bytes_written);
        }
      else
        {
          /* We need a bounce buffer. */
          if (bounce == NULL)
            {
              bounce = malloc (BLOCK_SECTOR_SIZE);
              if (bounce == NULL)
                break;
            }

          /* If the sector contains data before or after the chunk
             we're writing, then we need to read in the sector
             first.  Otherwise we start with a sector of all zeros. */
          if (sector_ofs > 0 || chunk_size < sector_left)
            block_read (fs_device, sector_idx, bounce);
          else
            memset (bounce, 0, BLOCK_SECTOR_SIZE);
          memcpy (bounce + sector_ofs, buffer + bytes_written, chunk_size);
          block_write (fs_device, sector_idx, bounce);
        }
        #endif

      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_written += chunk_size;
    }
  free (bounce);

  return bytes_written;
}

/* Disables writes to INODE.
   May be called at most once per inode opener. */
void
inode_deny_write (struct inode *inode)
{
  inode->deny_write_cnt++;
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
}

/* Re-enables writes to INODE.
   Must be called once by each inode opener who has called
   inode_deny_write() on the inode, before closing the inode. */
void
inode_allow_write (struct inode *inode)
{
  ASSERT (inode->deny_write_cnt > 0);
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
  inode->deny_write_cnt--;
}

/* Returns the length, in bytes, of INODE's data. */
off_t
inode_length (const struct inode *inode)
{
  return inode->data.length;
}
