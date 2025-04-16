#define WHITE 0
#define BLACK 1
#define NUM_PLAYERS 2

#define NUM_RANKS 8
#define NUM_FILES 8

#define NUM_BOARD_SQUARES (NUM_RANKS * NUM_FILES)

#define CHARS_IN_BOARD \
(NUM_BOARD_SQUARES / 2)  // 64 squares / 2 (nibbles per char)

#define PAWN_ID           1
#define ROOK_ID           2
#define KNIGHT_ID         3
#define BISHOP_ID         4
#define QUEEN_ID          5
#define KING_ID           6
#define NUM_PIECE_TYPES_0 6
#define EMPTY_ID          7

#define NUM_PIECES_PER_PLAYER 16

#define WIDTH_IN_PIXELS 50
#define HEIGHT_IN_PIXELS 50

#define SHRUNK_WIDTH_IN_PIXELS (WIDTH_IN_PIXELS / 2)
#define SHRUNK_HEIGHT_IN_PIXELS (HEIGHT_IN_PIXELS / 2)

#define BOARD_WIDTH (NUM_FILES * width_in_pixels)
#define BOARD_HEIGHT (NUM_RANKS * height_in_pixels)

#define NUM_PIECE_TYPES 5

#define BITS_PER_BOARD_SQUARE 4

#define FONT_HEIGHT 12
#define FONT_WIDTH 9

#define Y_PIXELS 200

#define MAX_ANNOTATION_LINE_LEN 25
#define MAX_ANNOTATION_LINES (Y_PIXELS / FONT_HEIGHT)

#define ANNOTATION_X (8 * XLEN + 2 + FONT_WIDTH)
#define ANNOTATION_Y 5

struct board_comparison {
  char orientation;
  unsigned char board[2][CHARS_IN_BOARD];  /* 8 columns * 8 rows / 2 (nibbles per char) */
  char file;
  char rank;
};
