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
//  group_bitmaps = (struct bitmap**)malloc(GROUP_COUNT * sizeof(struct bitmap*));
//  for(uint32_t i=0; i<GROUP_COUNT; i++)
//    group_bitmaps[i] = bitmap_create (GROUP_SIZE);
  if (free_map == NULL)
    PANIC ("bitmap creation failed--file system device is too large");
  bitmap_mark (free_map, FREE_MAP_SECTOR);
  bitmap_mark (free_map, ROOT_DIR_SECTOR);
//  for(uint32_t i=0; i<GROUP_COUNT; i++)
//    bitmap_mark (group_bitmaps[i], i+2);
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

//void
//bit_map_release (uint32_t group,struct bitmap** bit_map,block_sector_t sector, size_t cnt)
//{
//    ASSERT (bitmap_all (bit_map[group], sector, cnt));
//    bitmap_set_multiple (bit_map[group], sector, cnt, false);
//    bitmap_write (bit_map[group], bit_map_files[group]);
//}

/* Opens the free map file and reads it from disk. */
void
free_map_open (void)
{
  free_map_file = file_open (inode_open (FREE_MAP_SECTOR));
//  for(size_t i=0; i<GROUP_COUNT; i++)
//    bit_map_files[i] = file_open (inode_open (i+2));
  if (free_map_file == NULL)
    PANIC ("can't open free map");
//  for(size_t i=0; i<GROUP_COUNT; i++)
//    bitmap_read (group_bitmaps[i], bit_map_files[i]);
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
//  bit_map_files = (struct file**)malloc(GROUP_COUNT * sizeof(struct file*));
//  for(size_t i=0; i<GROUP_COUNT; i++)
//      bit_map_files[i] = NULL;
  if (!inode_create (FREE_MAP_SECTOR, bitmap_file_size (free_map)))
    PANIC ("free map creation failed");
//  for(size_t i=0; i<GROUP_COUNT; i++)
//    if(!inode_create (i+2, bitmap_file_size (group_bitmaps[i])))
//        PANIC ("group map creation failed");
  /* Write bitmap to file. */
  free_map_file = file_open (inode_open (FREE_MAP_SECTOR));
  if (free_map_file == NULL)
    PANIC ("can't open free map");
  if (!bitmap_write (free_map, free_map_file))
    PANIC ("can't write free map");
//  for(size_t i=0; i<GROUP_COUNT; i++)
//    bit_map_files[i] = file_open (inode_open (i+2));
//  for(size_t i=0; i<GROUP_COUNT; i++)
//    bitmap_write (group_bitmaps[i], bit_map_files[i]);
}
