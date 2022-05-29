#include <random.h>
#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

#define BUF_SIZE 10 * 512
#define BUFFER_C_SIZE 200 * 512
#define CHUNK_SIZE 512

char buf[BUF_SIZE];
char big_buf[BUFFER_C_SIZE];

const char *file_name = "big";

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
  unsigned write_cnt1, write_cnt2;
  
  CHECK (create (file_name, 0), "create \"%s\"", file_name);

  read_cnt1 = block_read_count ();
  write_cnt1 = block_write_count ();

  empty_cache ();

  read_cnt2 = block_read_count ();
  write_cnt2 = block_write_count ();

  unsigned read_diff, write_diff;

  read_diff = read_cnt2 - read_cnt2;
  write_diff = write_cnt2 - write_cnt1;

  if (write_diff > 270)
    {
      msg ("too many writes");
    }
  if (read_diff > 270)
    {
      msg ("too many reads");
    }
  if (read_diff <= 270 && write_diff <= 270)
    {
      msg ("cache ok");
    }
}
