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

int read_board_comparisons(char *filename,int *num_comparisons_pt,struct board_comparison **comparisons_pt)
{
  struct stat statbuf;
  int bytes_to_read;
  int bytes_read;
  int struct_size;
  struct board_comparison *comparisons;
  int fhndl;

  if (stat(filename,&statbuf) == -1)
    return 1;

  bytes_to_read = (int)statbuf.st_size;
  struct_size = sizeof(struct board_comparison);

  if (bytes_to_read % struct_size)
    return 2;

  *num_comparisons_pt = bytes_to_read / struct_size;

  if ((comparisons = (struct board_comparison *)malloc(bytes_to_read)) == NULL)
    return 3;

  if ((fhndl = open(filename,O_RDONLY | O_BINARY)) == -1) {
    free(comparisons);
    return 4;
  }

  bytes_read = read(fhndl,(char *)comparisons,bytes_to_read);

  if (bytes_read != bytes_to_read) {
    free(comparisons);
    close(fhndl);
    return 5;
  }

  close(fhndl);

  *comparisons_pt = comparisons;

  return 0;
}
