#include <random.h>
#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

#define BUF_SIZE 20 * 512
#define BUFFER_C_SIZE 64 * 512
#define CHUNK_SIZE 512

char buf[BUF_SIZE];
char big_buf[BUFFER_C_SIZE];

const char *file_name = "big";
const char *file_name2 = "small";

void
empty_cache ()
{
	size_t ofs;
  	int fd;
	CHECK ((fd = open (file_name)) > 1, "open \"%s\"", file_name);
	random_bytes (big_buf, sizeof big_buf);
  	quiet = true;
  	for (ofs = 0; ofs < BUFFER_C_SIZE; ofs += CHUNK_SIZE)
		CHECK (write (fd, big_buf + ofs, CHUNK_SIZE) > 0,
	       "write %d bytes at offset %zu in \"%s\"",
	       (int) CHUNK_SIZE, ofs, file_name);
  	quiet = false;
  	
  	close (fd);
}

void
test_main (void)
{
  size_t ofs;
  int fd;
  
  unsigned read_cnt1, read_cnt2, read_cnt3, read_cnt4;
  
  CHECK (create (file_name, 0), "create \"%s\"", file_name);

  CHECK (create (file_name2, 0), "create \"%s\"", file_name2);
  CHECK ((fd = open (file_name2)) > 1, "open \"%s\"", file_name2);
  
  random_bytes (buf, sizeof buf);
  quiet = true;
  for (ofs = 0; ofs < BUF_SIZE; ofs += CHUNK_SIZE)
    CHECK (write (fd, buf + ofs, CHUNK_SIZE) > 0,
           "write %d bytes at offset %zu in \"%s\"",
           (int) CHUNK_SIZE, ofs, file_name);
  quiet = false;
  
  close (fd);

  empty_cache ();
  
//  CHECK (create (file_name2, 0), "create \"%s\"", file_name2;
  CHECK ((fd = open (file_name2)) > 1, "open \"%s\"", file_name2);
  
  read_cnt1 = block_read_count ();
  
  quiet = true;
  for (ofs = 0; ofs < BUF_SIZE; ofs += CHUNK_SIZE)
    CHECK (read (fd, buf + ofs, CHUNK_SIZE) > 0,
           "read %d bytes at offset %zu in \"%s\"",
           (int) CHUNK_SIZE, ofs, file_name);
  quiet = false;
  
  
  read_cnt2 = block_read_count ();
  
  close (fd);
  
  
  
  CHECK ((fd = open (file_name2)) > 1, "open \"%s\"", file_name2);
  
  read_cnt3 = block_read_count ();
  
  quiet = true;
  for (ofs = 0; ofs < BUF_SIZE; ofs += CHUNK_SIZE)
    CHECK (read (fd, buf + ofs, CHUNK_SIZE) > 0,
           "read %d bytes at offset %zu in \"%s\"",
           (int) CHUNK_SIZE, ofs, file_name);
  quiet = false;
  
  
  read_cnt4 = block_read_count ();
  
  close (fd);

  unsigned wo_cache, w_cache;
  wo_cache = read_cnt2 - read_cnt1;
  w_cache = read_cnt4 - read_cnt3;

  if (w_cache < wo_cache - 10)
    {
      msg ("cache ok");
    }
  else
    {
      msg ("cache not ok");
    }
  // msg ("User test finished. a:%u b:%u c:%u d:%u", 
  // read_cnt1, read_cnt2, read_cnt3, read_cnt4);

  /*random_bytes (buf, sizeof buf);
  quiet = true;
  for (ofs = 0; ofs < BUF_SIZE; ofs += CHUNK_SIZE)
    CHECK (write (fd, buf + ofs, CHUNK_SIZE) > 0,
           "write %d bytes at offset %zu in \"%s\"",
           (int) CHUNK_SIZE, ofs, file_name);
  quiet = false;

  wait_children (children, CHILD_CNT);*/
}
