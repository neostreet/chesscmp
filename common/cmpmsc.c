#include <stdio.h>
#include "cmp.h"
#include "cmp.glb"
#include "cmp.fun"
#include "cmp.mac"
#include "bitfuns.h"

int format_square(int square)
{
  bool bBlack;
  int return_char;

  if (!square)
    return (int)'.';

  if (square < 0) {
    bBlack = true;
    square *= -1;
  }
  else
    bBlack = false;

  if (square == 1)
    return_char = 'P';
  else
    return_char = piece_ids[square - 2];

  if (!bBlack)
    return_char += ('a' - 'A');

  return return_char;
}

void print_bd0(unsigned char *board,int orientation)
{
  int m;
  int n;
  int square;

  for (m = 0; m < NUM_RANKS; m++) {
    for (n = 0; n < NUM_FILES; n++) {
      if (!orientation)
        square = get_piece2(board,(NUM_RANKS - 1) - m,n);
      else
        square = get_piece2(board,m,(NUM_FILES - 1) - n);

      printf("%c",format_square(square));

      if (n < (NUM_FILES - 1))
        putchar(' ');
    }

    putchar(0x0a);
  }
}

int get_piece1(unsigned char *board,int board_offset)
{
  unsigned int bit_offset;
  unsigned short piece;
  int piece_int;

  bit_offset = board_offset * BITS_PER_BOARD_SQUARE;

  piece = get_bits(BITS_PER_BOARD_SQUARE,board,bit_offset);
  piece_int = piece;

  if (piece & 0x8)
    piece_int |= 0xfffffff0;

  return piece_int;
}

int get_piece2(unsigned char *board,int rank,int file)
{
  int board_offset;

  board_offset = rank * NUM_FILES + file;
  return get_piece1(board,board_offset);
}

static int set_piece_calls;
static int dbg_set_piece_call;
static int dbg_board_offset;
static int dbg_piece;

void set_piece1(unsigned char *board,int board_offset,int piece)
{
  unsigned int bit_offset;
  int dbg;

  set_piece_calls++;

  if (dbg_set_piece_call == set_piece_calls)
    dbg = 0;

  if (debug_level == 2) {
    if (debug_fptr != NULL)
      fprintf(debug_fptr,"set_piece: board_offset = %d, piece %d\n",board_offset,piece);
  }

  bit_offset = board_offset * BITS_PER_BOARD_SQUARE;
  set_bits(BITS_PER_BOARD_SQUARE,board,bit_offset,piece);
}

void set_piece2(unsigned char *board,int rank,int file,int piece)
{
  int board_offset;

  board_offset = rank * NUM_FILES + file;
  set_piece1(board,board_offset,piece);
}
