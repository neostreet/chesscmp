#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include "cmp.h"
#include "cmp.fun"
#include "cmp.glb"
#include "cmp.mac"
#include "bitfuns.h"

static unsigned char initial_board[] = {
  (unsigned char)0x23, (unsigned char)0x45, (unsigned char)0x64, (unsigned char)0x32,
  (unsigned char)0x11, (unsigned char)0x11, (unsigned char)0x11, (unsigned char)0x11,
  (unsigned char)0x00, (unsigned char)0x00, (unsigned char)0x00, (unsigned char)0x00,
  (unsigned char)0x00, (unsigned char)0x00, (unsigned char)0x00, (unsigned char)0x00,
  (unsigned char)0x00, (unsigned char)0x00, (unsigned char)0x00, (unsigned char)0x00,
  (unsigned char)0x00, (unsigned char)0x00, (unsigned char)0x00, (unsigned char)0x00,
  (unsigned char)0xff, (unsigned char)0xff, (unsigned char)0xff, (unsigned char)0xff,
  (unsigned char)0xed, (unsigned char)0xcb, (unsigned char)0xac, (unsigned char)0xde
};

int populate_random_sample_ixs(int sample_size,int *random_sample_ixs);

int get_piece_type_ix(int chara)
{
  int n;

  for (n = 0; n < NUM_PIECE_TYPES; n++)
    if (chara == piece_ids[n])
      return n;

  return 0; /* should never happen */
}

void set_initial_board(unsigned char *board)
{
  int n;

  for (n = 0; n < CHARS_IN_BOARD; n++)
    board[n] = initial_board[n];
}

int read_board_comparisons(
  char *filename,
  int *num_comparisons_pt,
  struct board_comparison **comparisons_pt,
  int **comparison_ixs_pt)
{
  int n;
  struct stat statbuf;
  int bytes_to_read;
  int bytes_read;
  int struct_size;
  struct board_comparison *comparisons;
  int bytes_to_malloc;
  int *comparison_ixs;
  int fhndl;
  int retval;

  if (stat(filename,&statbuf) == -1)
    return 1;

  bytes_to_read = (int)statbuf.st_size;
  struct_size = sizeof(struct board_comparison);

  if (bytes_to_read % struct_size)
    return 2;

  *num_comparisons_pt = bytes_to_read / struct_size;

  if ((comparisons = (struct board_comparison *)malloc(bytes_to_read)) == NULL)
    return 3;

  bytes_to_malloc = *num_comparisons_pt * sizeof(int);

  if ((comparison_ixs = (int *)malloc(bytes_to_malloc)) == NULL) {
    free(comparisons);
    return 4;
  }

  if ((fhndl = open(filename,O_RDONLY | O_BINARY)) == -1) {
    free(comparisons);
    free(comparison_ixs);
    return 5;
  }

  bytes_read = read(fhndl,(char *)comparisons,bytes_to_read);

  if (bytes_read != bytes_to_read) {
    free(comparisons);
    free(comparison_ixs);
    close(fhndl);
    return 6;
  }

  close(fhndl);

  retval = populate_random_sample_ixs(*num_comparisons_pt,comparison_ixs);

  if (retval) {
    for (n = 0; n < *num_comparisons_pt; n++)
      comparison_ixs[n] = n;
  }

  *comparisons_pt = comparisons;
  *comparison_ixs_pt = comparison_ixs;

  return 0;
}

int populate_random_sample_ixs(int sample_size,int *random_sample_ixs)
{
  int m;
  int n;
  int work;
  int mem_amount;
  int *random_sample_hits;
  int curr_sample_size;
  int unused_count;

  mem_amount = sample_size * sizeof(int *);

  if ((random_sample_hits = (int *)malloc(mem_amount)) == NULL)
    return 1;

  curr_sample_size = sample_size;

  for (n = 0; n < sample_size; n++)
    random_sample_hits[n] = 0;

  for (n = 0; n < sample_size; n++) {
    work = rand();
    work %= curr_sample_size;
    work++;
 
    unused_count = 0;
 
    for (m = 0; m < sample_size; m++) {
      if (!random_sample_hits[m]) {
        unused_count++;
 
        if (unused_count == work) {
          random_sample_hits[m] = 1;
          break;
        }
      }
    }
 
    random_sample_ixs[n] = m;
 
    curr_sample_size--;
  }

  free(random_sample_hits);
 
  return 0;
}
