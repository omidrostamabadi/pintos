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
#define CACHE_BYPASS
//#define NO_CLOCK_ALG
#define CLOCK_CHANCES 2

/* Identifies an inode. */
#define INODE_MAGIC 0x494e4f44

/* Number of cache elements */
#define CACHE_ELEMENTS 64

#define PTR_PER_SECTOR (BLOCK_SECTOR_SIZE  / 4)



/* Returns the number of sectors to allocate for an inode SIZE
   bytes long. */
static inline size_t
bytes_to_sectors (off_t size)
{
  return DIV_ROUND_UP (size, BLOCK_SECTOR_SIZE);
}



/* Data structures, global, static variables for buffer cache system */ 

/* Entries stored in cache */
struct cache_entry
  {
    struct inode *its_inode; /* Inode for this cached sector */
    int used; /* Is set anytime there's an access to this entry */
    int dirty; /* If set, memory is inconsistent with disk */
    int is_busy;
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

int clock_hand = 0; /* Used in clock algorithm */

struct cache_entry cache_entries[CACHE_ELEMENTS]; /* Buffer cache */

struct lock cache_lock; /* Acquire this lock prior to any modification to cache */

struct list busy_sectors; /* List of sectors that should not be accessed by
  other threads in the system */ 

/* Helper functions for buffer cache system */

/* Checks cache to see if the sector is cached.
    Returns the index for the cache element containing the sector if found.
    If cannot find the sector in cache, returns -1.
    */
static int
check_cache (block_sector_t sector)
{
  for (int i = 0; i < CACHE_ELEMENTS; i++)
    {
      if (cache_entries[i].sector == sector && cache_entries[i].is_busy == 1)
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

  struct cache_entry *this_sector = &cache_entries[cache_index];

  lock_release (&cache_lock);
  lock_acquire (& (this_sector->busy_lock) );
  lock_acquire (&cache_lock);
  // if (this_sector != NULL)
  //   {
  //     lock_release (&cache_lock);
  //     lock_acquire (& (this_sector->busy_lock));
  //     lock_acquire (&cache_lock);
  //   }
  // else
  //   {
  //     this_sector = & (cache_entries[cache_index]);
  //     lock_acquire (&this_sector->busy_lock);
  //   }  
  // list_push_back (&busy_sectors, & (this_sector->busy_elem) );
}

static int
add_to_cache(block_sector_t sector)
{
  lock_acquire (&cache_lock);

  int cache_index = check_cache(sector);

  if (cache_index == -1)
    {
      cache_index = clock_evict_block ();
      lock_release (&cache_lock);
      fetch_new_block (cache_index, sector);
      return cache_index;
    }

  struct cache_entry *this_sector = &cache_entries[cache_index];

  lock_release (&cache_lock);
  lock_acquire (& (this_sector->busy_lock) );
  lock_acquire (&cache_lock);

  while (this_sector->sector != sector)
    {
  lock_release (& (this_sector->busy_lock) );
  cache_index = check_cache (sector);
  if (cache_index == -1)
    {
      cache_index = clock_evict_block ();
      lock_release (&cache_lock);
      fetch_new_block (cache_index, sector);
      return cache_index;
    }
  this_sector = &cache_entries[cache_index];

  lock_release (&cache_lock);
  lock_acquire (& (this_sector->busy_lock) );
  lock_acquire (&cache_lock);
  }
  lock_release (&cache_lock);
  return cache_index;
}

static int
simple_clock ()
{
  #ifdef NO_CLOCK_ALG
  int to_be_evicted = clock_hand;
  clock_hand++;
  if (clock_hand >= CACHE_ELEMENTS)
    clock_hand = 0;
  return to_be_evicted;
  #else
  struct cache_entry *tmp;
  while (1)
    {
      tmp = &cache_entries[clock_hand];
      if (tmp->used != 0)
        {
          tmp->used--;
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
          return to_be_evicted;
        }
    }
    #endif
}

static int
simple_add_cache (block_sector_t sector)
{
  int cache_index = check_cache (sector);
  if (cache_index == -1)
    {
      cache_index = simple_clock ();
      if (cache_entries[cache_index].dirty == 1)
        {
          block_write (fs_device, cache_entries[cache_index].sector, 
            cache_entries[cache_index].in_mem_data);
        }
      block_read (fs_device, sector, cache_entries[cache_index].in_mem_data);
      cache_entries[cache_index].used = CLOCK_CHANCES;
      cache_entries[cache_index].dirty = 0;
      cache_entries[cache_index].is_busy = 1;
      cache_entries[cache_index].its_inode = NULL;
      cache_entries[cache_index].sector = sector;
      // clock_hand++;
      // if (clock_hand >= CACHE_ELEMENTS)
      //   clock_hand = 0;
    }
  cache_entries[cache_index].used = CLOCK_CHANCES;
  return cache_index;
}

/* Choose a cache block to be evicted in order to make room for a new one.
    Works based on clock algorithm */
static int
clock_evict_block ()
{
  #ifdef NO_CLOCK_ALG
  int to_be_evicted = clock_hand;
  clock_hand++;
  if (clock_hand >= CACHE_ELEMENTS)
    clock_hand = 0;
  add_to_busy (to_be_evicted);
  return to_be_evicted;
  #else
  struct cache_entry *tmp;
  while (1)
    {
      tmp = &cache_entries[clock_hand];
      if (tmp->used != 0)
        {
          tmp->used--;
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
          if (cache_entries[to_be_evicted].used == 0)
            return to_be_evicted;
          else
            lock_release (& (cache_entries[to_be_evicted].busy_lock));
        }
    }
    #endif
}

/* Replaces sector_index sector with cache_entry in cache_index position.
    Writes previous data to disk if it is dirty. */
static void 
fetch_new_block (int cache_index, block_sector_t sector_index)
{
  ASSERT (lock_held_by_current_thread (& (cache_entries[cache_index].busy_lock) ));
  if (cache_entries[cache_index].dirty == 1)
    {
      block_write (fs_device, cache_entries[cache_index].sector, 
                    cache_entries[cache_index].in_mem_data);
    }
  block_read (fs_device, sector_index, cache_entries[cache_index].in_mem_data);
  cache_entries[cache_index].dirty = 0;
  cache_entries[cache_index].used = CLOCK_CHANCES;
  cache_entries[cache_index].is_busy = 1;
  cache_entries[cache_index].its_inode = NULL;
  cache_entries[cache_index].sector = sector_index;
}

/* Returns the address of struct busy_sector in busy_sectors list.
    If sector is not busy, returns NULL. */
static struct cache_entry *
check_busy (int cache_index)
{
  struct list_elem *e;
  struct cache_entry tmp = cache_entries[cache_index];
  for (e = list_begin (&busy_sectors); e != list_end (&busy_sectors);
       e = list_next (e))
    {
      struct cache_entry *bs = list_entry (e, struct cache_entry, busy_elem);
      if (bs->sector == tmp.sector && bs->its_inode == tmp.its_inode)
        return bs;
    }
  return NULL;
}

static void
cache_read (block_sector_t sector, void *buffer, int sector_ofs, int chunk_size)
{
  #ifndef CACHE_BYPASS
  int cache_index = add_to_cache (sector);
  memcpy (buffer, cache_entries[cache_index].in_mem_data + sector_ofs, chunk_size);
  lock_release (& (cache_entries[cache_index].busy_lock) );
  #else
  normal_read (sector, buffer, sector_ofs, chunk_size);
  #endif
}

static void
cache_write (block_sector_t sector, void *buffer, int sector_ofs, int chunk_size)
{
  #ifndef CACHE_BYPASS
  int cache_index = add_to_cache (sector);
  memcpy (cache_entries[cache_index].in_mem_data + sector_ofs, buffer, chunk_size);
  cache_entries[cache_index].dirty = 1;
  lock_release (& (cache_entries[cache_index].busy_lock) );
  #else
  normal_write (sector, buffer, sector_ofs, chunk_size);
  #endif
}

static void
normal_read (block_sector_t sector, void *buffer, int sector_ofs, int chunk_size)
{
  char *bounce = NULL;
  if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
    {
      /* Read full sector directly into caller's buffer. */
      block_read (fs_device, sector, buffer);
    }
  else
    {
      /* Read sector into bounce buffer, then partially copy
          into caller's buffer. */
      if (bounce == NULL)
        {
          bounce = malloc (BLOCK_SECTOR_SIZE);
          if (bounce == NULL)
            ASSERT (bounce != NULL);;
        }
      block_read (fs_device, sector, bounce);
      memcpy (buffer, bounce + sector_ofs, chunk_size);
    }
  free(bounce);
}

static void
normal_write (block_sector_t sector, void *buffer, int sector_ofs, int chunk_size)
{
  char *bounce = NULL;
  int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
  if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
    {
      /* Write full sector directly to disk. */
      block_write (fs_device, sector, buffer);
    }
  else
    {
      /* We need a bounce buffer. */
      if (bounce == NULL)
        {
          bounce = malloc (BLOCK_SECTOR_SIZE);
          if (bounce == NULL)
            ASSERT (bounce != NULL);
        }

      /* If the sector contains data before or after the chunk
          we're writing, then we need to read in the sector
          first.  Otherwise we start with a sector of all zeros. */
      if (sector_ofs > 0 || chunk_size < sector_left)
        block_read (fs_device, sector, bounce);
      else
        memset (bounce, 0, BLOCK_SECTOR_SIZE);
      memcpy (bounce + sector_ofs, buffer, chunk_size);
      block_write (fs_device, sector, bounce);
    }
  free (bounce);
}

/* Returns the block device sector that contains byte offset POS
   within INODE.
   Returns -1 if INODE does not contain data for a byte at offset
   POS. */
static block_sector_t
byte_to_sector (const struct inode *inode, off_t pos)
{
  ASSERT (inode != NULL);
  if (pos < inode->data.length) {
      if(pos < 123 * BLOCK_SECTOR_SIZE)
        return inode->data.direct_ptr[pos / BLOCK_SECTOR_SIZE];
      else if (pos < (123+PTR_PER_SECTOR) * BLOCK_SECTOR_SIZE){
        block_sector_t buf[PTR_PER_SECTOR];
        cache_read(inode->data.indirect_ptr,buf, 0, BLOCK_SECTOR_SIZE);
        return buf[(pos-123*BLOCK_SECTOR_SIZE) / BLOCK_SECTOR_SIZE];
      }else {
        off_t indirect_pos = (pos - (123+PTR_PER_SECTOR) * BLOCK_SECTOR_SIZE)/(BLOCK_SECTOR_SIZE*PTR_PER_SECTOR);
        off_t indirect_off = (pos - (123+PTR_PER_SECTOR) * BLOCK_SECTOR_SIZE)%(BLOCK_SECTOR_SIZE*PTR_PER_SECTOR);
        block_sector_t buf[PTR_PER_SECTOR];
        cache_read(inode->data.dbl_indirect_ptr,buf, 0, BLOCK_SECTOR_SIZE);
        cache_read(buf[indirect_pos],buf,0,BLOCK_SECTOR_SIZE);
        return buf[indirect_off / BLOCK_SECTOR_SIZE];
      }
  }
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
  lock_init (&cache_lock);
  list_init (&busy_sectors);

  for (int i = 0; i < CACHE_ELEMENTS; i++)
    {
      lock_init (& (cache_entries[i].busy_lock) );
      cache_entries[i].dirty = 0;
      cache_entries[i].is_busy = 0;
    }
}

uint32_t find_prefer_group(){
    return 0;
}

bool
create_indirect_sector (block_sector_t* indirect_ptr, size_t size) {
    block_sector_t temp_sector;
    bool success = false;
    if (group_free_map_allocate(find_prefer_group(), 1, indirect_ptr)) {
        size_t i;
        for (i = 0; i < size; i++) {
            if (group_free_map_allocate(find_prefer_group(),1, &temp_sector)) {
                static char zeros[BLOCK_SECTOR_SIZE];
                cache_write(*indirect_ptr,&temp_sector,4*i, sizeof(block_sector_t*));
                cache_write(temp_sector,zeros, 0, BLOCK_SECTOR_SIZE);
                success = true;
            }else{
                success = false;
                break;
            }
        }
    }
    return success;
}


bool
create_indirect_sector_from_ofs (block_sector_t* indirect_ptr, size_t size, off_t offset) {
    block_sector_t temp_sector;
    bool success = false,flag=true;
    if(*indirect_ptr==0) {
        flag = group_free_map_allocate(find_prefer_group(), 1, indirect_ptr);
    }
    if (flag) {
        size_t i;
        for (i = offset; i < size+offset; i++) {
            if (group_free_map_allocate(find_prefer_group(),1, &temp_sector)) {
                static char zeros[BLOCK_SECTOR_SIZE];
                cache_write(*indirect_ptr,&temp_sector,4*i, sizeof(block_sector_t*));
                cache_write(temp_sector,zeros, 0, BLOCK_SECTOR_SIZE);
                success = true;
            }else{
                success = false;
                break;
            }
        }
    }
    return success;
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
        if (length==0) {
            cache_write(sector,disk_inode,0, BLOCK_SECTOR_SIZE);
            return true;
        }
        size_t min_size = sectors < 123 ? sectors : 123;
        size_t i;
        for (i = 0; i <= min_size; i++) {
            if(min_size != 0 && min_size == i)
                break;
            if (group_free_map_allocate(find_prefer_group(),1, &disk_inode->direct_ptr[i])) {
                success = true;
//                cache_write(sector,disk_inode,0, BLOCK_SECTOR_SIZE);
                if (sectors > 0) {
                    static char zeros[BLOCK_SECTOR_SIZE];
                    cache_write(disk_inode->direct_ptr[i],zeros,0, BLOCK_SECTOR_SIZE);
                }
            }else{
                success = false;
            }
        }
        if (sectors > 123){
            size_t min_size = sectors - 123 < PTR_PER_SECTOR ? sectors - 123 : PTR_PER_SECTOR;
            success = create_indirect_sector(&disk_inode->indirect_ptr,min_size);
        }
        if (sectors > 123 + PTR_PER_SECTOR){
            if (group_free_map_allocate(find_prefer_group(), 1, &disk_inode->dbl_indirect_ptr)) {
                int size = sectors - 123 - PTR_PER_SECTOR;
                size_t i=0;
                while(size>0){
                    block_sector_t temp_sector;
                    size_t min_size = size < PTR_PER_SECTOR ? size : PTR_PER_SECTOR;
                    success = create_indirect_sector(&temp_sector, min_size);
                    if (!success)
                        break;
                    cache_write(disk_inode->dbl_indirect_ptr,&temp_sector,4*i, sizeof(block_sector_t*));
                    size -= PTR_PER_SECTOR;
                    i++;
                }

            }
        }
        if (success){
            cache_write(sector,disk_inode,0, BLOCK_SECTOR_SIZE);
            disk_inode->start = disk_inode->direct_ptr[0];
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
  //block_read (fs_device, inode->sector, &inode->data);
  cache_read (inode->sector, &inode->data, 0, BLOCK_SECTOR_SIZE);
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
          size_t i;
          size_t sectors = bytes_to_sectors(inode->data.length);
          size_t size_left = inode->data.length;
          off_t cur_byte = 0;
          for(i=0; i < sectors; i++) {
              block_sector_t release_sector = byte_to_sector(inode,cur_byte);
              free_map_release(release_sector,1);
              cur_byte += BLOCK_SECTOR_SIZE;
          }
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

      #ifndef CACHE_BYPASS2
      /* Check the cache for this sector. If the sector is busy, wait 
          until it's free and add it to busy again. If the sector is not
          present in the cache, read it from the disk. After these several lines
          of code, cache_index points to a valid cache block for the sector. */
      //lock_acquire (&cache_lock);

      //int cache_index = add_to_cache (sector_idx, inode);
      //int cache_index = simple_add_cache (sector_idx);

      cache_read (sector_idx, buffer + bytes_read, sector_ofs, chunk_size);

      // int cache_index = check_cache (sector_idx, inode);
      // if (cache_index != -1) /* Sector found in cache */
      //   {
      //     add_to_busy (cache_index);
      //     lock_release (&cache_lock);
      //   }
      // else /* Could not find the sector in cache */
      //   {
      //     cache_index = clock_evict_block ();
      //     lock_release (&cache_lock);
      //     fetch_new_block (cache_index, sector_idx, inode);
      //   }

      // memcpy (buffer + bytes_read, 
      //         cache_entries[cache_index].in_mem_data + sector_ofs, chunk_size);

      //lock_release (&cache_lock);
      /* We are done with this sector. Remove it from busy list */
    //  list_remove (& (cache_entries[cache_index].busy_elem) );
      //lock_release (& (cache_entries[cache_index].busy_lock) );
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

bool
allocate_sectors(struct inode* data_inode, block_sector_t needed_sectors,block_sector_t cur_sectors){
    block_sector_t remain_sectors = needed_sectors - cur_sectors;
    bool success = false;
    if(cur_sectors < 123){
        for(size_t i=cur_sectors; i < 123; i++){
            if (group_free_map_allocate(find_prefer_group(),1, &data_inode->data.direct_ptr[i])) {
//                cache_write(sector,disk_inode,0, BLOCK_SECTOR_SIZE);
                    static char zeros[BLOCK_SECTOR_SIZE];
                    cache_write(data_inode->data.direct_ptr[i],zeros,0, BLOCK_SECTOR_SIZE);
                    remain_sectors--;
                    if(remain_sectors==0)
                        return true;
            }
        }
    }
    if(cur_sectors < 123 + PTR_PER_SECTOR) {
        size_t min_size = remain_sectors < PTR_PER_SECTOR+123-cur_sectors ? remain_sectors : PTR_PER_SECTOR+123-cur_sectors;
        off_t ofs = cur_sectors<123 ? 0 : cur_sectors-123;
        success = create_indirect_sector_from_ofs(&data_inode->data.indirect_ptr, min_size,ofs);
        remain_sectors -= min_size;
        if (remain_sectors == 0)
            return true;
    }
    if (group_free_map_allocate(find_prefer_group(), 1, &data_inode->data.dbl_indirect_ptr)) {
        int size = needed_sectors - 123 - PTR_PER_SECTOR;
        size_t i=0;
        if(cur_sectors > 123 + PTR_PER_SECTOR){
            i = (cur_sectors -123 - PTR_PER_SECTOR)/PTR_PER_SECTOR;
        }
        block_sector_t sector_off = (cur_sectors -123 - PTR_PER_SECTOR) % PTR_PER_SECTOR;
        block_sector_t temp_sector;
        size_t min_size = size < PTR_PER_SECTOR-sector_off ? size : PTR_PER_SECTOR-sector_off;
        success = create_indirect_sector_from_ofs(&temp_sector, min_size,sector_off);
        if (!success)
            return false;
        remain_sectors-=min_size;
        if(remain_sectors==0)
            return true;
        cache_write(data_inode->data.dbl_indirect_ptr,&temp_sector,4*i, sizeof(block_sector_t*));
        size -= PTR_PER_SECTOR-sector_off;
        i++;
        while(size>0){
            block_sector_t temp_sector;
            size_t min_size = size < PTR_PER_SECTOR ? size : PTR_PER_SECTOR;
            success = create_indirect_sector(&temp_sector, min_size);
            if (!success)
                break;
            remain_sectors-=min_size;
            cache_write(data_inode->data.dbl_indirect_ptr,&temp_sector,4*i, sizeof(block_sector_t*));
            size -= PTR_PER_SECTOR;
            i++;
        }
    }
    if(remain_sectors==0)
        return true;
    else
        return false;
}
bool
extend_inode(struct inode* data_inode,off_t offset,size_t size){
    block_sector_t cur_sectors = bytes_to_sectors(data_inode->data.length);
    block_sector_t needed_sectors = bytes_to_sectors(offset+size);
    if (needed_sectors > cur_sectors){
        if(allocate_sectors(data_inode,needed_sectors,cur_sectors)){
            data_inode->data.length = offset+size;
            cache_write(data_inode->sector,&data_inode->data,0,BLOCK_SECTOR_SIZE);
            return true;
        }
        return false;
    }
    if(data_inode->data.length < offset+size ) {
        data_inode->data.length = offset + size;
        cache_write(data_inode->sector, &data_inode->data, 0, BLOCK_SECTOR_SIZE);
    }
    return true;
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

    extend_inode(inode,offset,size);
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

      #ifndef CACHE_BYPASS2
      /* Check the cache for this sector. If the sector is busy, wait 
          until it's free and add it to busy again. If the sector is not
          present in the cache, read it from the disk. After these several lines
          of code, cache_index points to a valid cache block for the sector. */
      //lock_acquire (&cache_lock);

      //int cache_index = add_to_cache (sector_idx, inode);
      //int cache_index = simple_add_cache (sector_idx);

      cache_write (sector_idx, buffer + bytes_written, sector_ofs, chunk_size);
      
      // int cache_index = check_cache (sector_idx, inode);
      // if (cache_index != -1) /* Sector found in cache */
      //   {
      //     add_to_busy (cache_index);
      //     lock_release (&cache_lock);
      //   }
      // else /* Could not find the sector in cache */
      //   {
      //     cache_index = clock_evict_block ();
      //     lock_release (&cache_lock);
      //     fetch_new_block (cache_index, sector_idx, inode);
      //   }
      
      // memcpy (cache_entries[cache_index].in_mem_data + sector_ofs, 
      //     buffer + bytes_written, chunk_size);
        
      
      // cache_entries[cache_index].dirty = 1;

      //lock_release (&cache_lock);
      // block_write (fs_device, cache_entries[cache_index].sector,
      //              cache_entries[cache_index].in_mem_data); // Write-through
      /* We are done with this sector. Remove it from busy list */
    //  list_remove (& (cache_entries[cache_index].busy_elem) );
      //lock_release (& (cache_entries[cache_index].busy_lock) );
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
