#include "filesys/free-map.h"
#include <bitmap.h>
#include <debug.h>
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "filesys/inode.h"

static struct file *free_map_file;   /* Free map file. */
static struct file **bit_map_files;   /* Free map file. */
static struct bitmap *free_map;      /* Free map, one bit per sector. */

/* Initializes the free map. */
void
free_map_init (void)
{
  free_map = bitmap_create (block_size (fs_device));
  if (free_map == NULL)
    PANIC ("bitmap creation failed--file system device is too large");
  bitmap_mark (free_map, FREE_MAP_SECTOR);
  bitmap_mark (free_map, ROOT_DIR_SECTOR);
}

/* Allocates CNT consecutive sectors from the free map and stores
   the first into *SECTORP.
   Returns true if successful, false if not enough consecutive
   sectors were available or if the free_map file could not be
   written. */
bool
free_map_allocate (size_t cnt, block_sector_t *sectorp)
{
  block_sector_t sector = bitmap_scan_and_flip (free_map, 0, cnt, false);
  if (sector != BITMAP_ERROR
      && free_map_file != NULL
      && !bitmap_write (free_map, free_map_file))
    {
      bitmap_set_multiple (free_map, sector, cnt, false);
      sector = BITMAP_ERROR;
    }
  if (sector != BITMAP_ERROR)
    *sectorp = sector;
  return sector != BITMAP_ERROR;
}

bool
group_free_map_allocate (uint32_t group_idx,size_t cnt, block_sector_t *sectorp)
{
    block_sector_t sector = group_bitmap_scan_and_flip (free_map, group_idx*GROUP_SIZE, (group_idx+1)*GROUP_SIZE-1, cnt, false);
    if (sector != BITMAP_ERROR
        && free_map_file != NULL
        && !bitmap_write (free_map, free_map_file))
    {
        bitmap_set_multiple (free_map, sector, cnt, false);
        sector = BITMAP_ERROR;
    }
    if (sector != BITMAP_ERROR)
        *sectorp = sector;
    return sector != BITMAP_ERROR;
}

bool
group_has_free (uint32_t group_idx)
{
  size_t start = group_idx * GROUP_SIZE;
  /* If this group has free elements */
  bool tmp = bitmap_contains (free_map, start, GROUP_SIZE, false);
  return tmp;
}

uint32_t
max_free_group ()
{
  uint32_t max_group = 0;
  size_t max_sectors = 0;
  size_t tmp_max;
  uint32_t start;
  for(uint32_t group_idx = 0; group_idx < GROUP_COUNT; group_idx++)
    {
      start = group_idx * GROUP_SIZE;
      tmp_max = bitmap_count (free_map, start, GROUP_SIZE, false);
      if (tmp_max > max_sectors)
        {
          max_sectors = tmp_max;
          max_group = group_idx;
        }
    }
  if (max_sectors != 0)
    return max_group;
  else
    return BITMAP_ERROR;
}

bool
bit_map_allocate (uint32_t group,struct bitmap **bit_map,size_t cnt, block_sector_t *sectorp)
{
    block_sector_t sector = bitmap_scan_and_flip (bit_map[group], 0, cnt, false);
    if (sector != BITMAP_ERROR
        && bit_map_files[group] != NULL
        && !bitmap_write (bit_map[group], bit_map_files[group]))
    {
        bitmap_set_multiple (bit_map[group], sector, cnt, false);
        sector = BITMAP_ERROR;
    }
    if (sector != BITMAP_ERROR)
        *sectorp = sector;
    return sector != BITMAP_ERROR;
}


/* Makes CNT sectors starting at SECTOR available for use. */
void
free_map_release (block_sector_t sector, size_t cnt)
{
  ASSERT (bitmap_all (free_map, sector, cnt));
  bitmap_set_multiple (free_map, sector, cnt, false);
  bitmap_write (free_map, free_map_file);
}

/* Opens the free map file and reads it from disk. */
void
free_map_open (void)
{
  free_map_file = file_open (inode_open (FREE_MAP_SECTOR));
  if (free_map_file == NULL)
    PANIC ("can't open free map");
  if (!bitmap_read (free_map, free_map_file))
    PANIC ("can't read free map");
}

/* Writes the free map to disk and closes the free map file. */
void
free_map_close (void)
{
  file_close (free_map_file);
}

/* Creates a new free map file on disk and writes the free map to
   it. */
void
free_map_create (void)
{
  /* Create inode. */
  if (!inode_create (FREE_MAP_SECTOR, bitmap_file_size (free_map)))
    PANIC ("free map creation failed");
  /* Write bitmap to file. */
  free_map_file = file_open (inode_open (FREE_MAP_SECTOR));
  if (free_map_file == NULL)
    PANIC ("can't open free map");
  if (!bitmap_write (free_map, free_map_file))
    PANIC ("can't write free map");
}
