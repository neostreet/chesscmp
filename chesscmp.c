#include <windows.h>
#include <commctrl.h> // includes the common control header
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "cmp.h"
#define MAKE_GLOBALS_HERE
#include "cmp.glb"
#include "cmp.fun"
#include "cmp.mac"
#include "chesscmp.h"
#include "resource.h"

static char appname[] = "Chesscmp";

static RECT spi_rect;

static int width_in_pixels;
static int height_in_pixels;

char couldnt_get_status[] = "couldn't get status of %s\n";
char couldnt_open[] = "couldn't open %s\n";

static char read_board_comparisons_failure[] = "read_board_comparisons() of %s failed: %d";

static char chesscmp_piece_bitmap_name[] = "BIGBMP";

static HANDLE chesscmp_piece_bitmap_handle;
static HDC hdc_compatible;

static OPENFILENAME OpenFileName;
static OPENFILENAME WriteFileName;
static TCHAR szCmpWriteFile[MAX_PATH];

static char chesscmp_filter[] = "\
Chesscmp files\0\
*.cmp\0\
All files (*.*)\0\
*.*\0\
\0\
\0\
";
static char chesscmp_ext[] = "cmp";

static TCHAR szCmpFile[MAX_PATH];

static int board_x_offset;
static int board_y_offset;

static int top_margin;
static int font_height;
static int left_margin;
static int bottom_margin;
static COLORREF bk_color;

static LOGFONT lf;
static HFONT hfont;

static int initial_x_pos;
static int initial_y_pos;

static int chesscmp_window_width;
static int chesscmp_window_height;

#define WINDOW_EXTRA_WIDTH  16
#define WINDOW_EXTRA_HEIGHT 61

static int window_extra_width;
static int window_extra_height;

// Makes it easier to determine appropriate code paths:
#if defined (WIN32)
   #define IS_WIN32 TRUE
#else
   #define IS_WIN32 FALSE
#endif
#define IS_NT      IS_WIN32 && (BOOL)(GetVersion() < 0x80000000)
#define IS_WIN32S  IS_WIN32 && (BOOL)(!(IS_NT) && \
(LOBYTE(LOWORD(GetVersion()))<4))
#define IS_WIN95 (BOOL)(!(IS_NT) && !(IS_WIN32S)) && IS_WIN32

#define COLOR_BLACK     0x00000000
#define COLOR_WHITE     0x00ffffff
#define DARK_GRAY 0x00a8a8a8
#define RED       0x000000ff
#define ORANGE    0x000088ff
#define YELLOW    0x0000ffff
#define GREEN     0x0000ff00
#define BLUE      0x00ff0000

#define CHARACTER_WIDTH   8
#define CHARACTER_HEIGHT 13

// Global Variables:

static HINSTANCE hInst;      // current instance
static HWND hWndToolBar;
static char szAppName[100];  // Name of the app
static char szTitle[100];    // The title bar text

static struct board_comparison *comparisons;
static int num_comparisons;
static int curr_comparison;

// Forward declarations of functions included in this code module:

BOOL InitApplication(HINSTANCE);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
char *trim_name(char *name);

LRESULT CALLBACK About(HWND, UINT, WPARAM, LPARAM);
BOOL CenterWindow (HWND, HWND);
void do_lbuttondown(HWND hWnd,int file,int rank);

//
//  FUNCTION: WinMain(HANDLE, HANDLE, LPSTR, int)
//
//  PURPOSE: Entry point for the application.
//
//  COMMENTS:
//
// This function initializes the application and processes the
// message loop.
//
int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
  int n;
  MSG msg;
  char *cpt;

  width_in_pixels = WIDTH_IN_PIXELS;
  height_in_pixels = HEIGHT_IN_PIXELS;

  highlight_rank = -1;
  highlight_file = -1;

  cpt = getenv("DEBUG_CHESSCMP");

  if (cpt != NULL) {
    debug_level = atoi(cpt);
    debug_fptr = fopen("chesscmp.dbg","w");
  }
  else {
    debug_level = 0;
    debug_fptr = NULL;
  }

  if ((cpt = getenv("TOP_MARGIN")) != NULL)
    top_margin = atoi(cpt);
  else
    top_margin = 16;

  font_height = top_margin;

  if ((cpt = getenv("LEFT_MARGIN")) != NULL)
    left_margin = atoi(cpt);
  else
    left_margin = 12;

  board_x_offset = left_margin;

  if ((cpt = getenv("BOTTOM_MARGIN")) != NULL)
    bottom_margin = atoi(cpt);
  else
    bottom_margin = 16;

  if ((cpt = getenv("BK_COLOR")) != NULL)
    sscanf(cpt,"%x",&bk_color);
  else
    bk_color = DARK_GRAY;

  if ((cpt = getenv("WINDOW_EXTRA_WIDTH")) != NULL)
    sscanf(cpt,"%d",&window_extra_width);
  else
    window_extra_width = WINDOW_EXTRA_WIDTH;

  if ((cpt = getenv("WINDOW_EXTRA_HEIGHT")) != NULL)
    sscanf(cpt,"%d",&window_extra_height);
  else
    window_extra_height = WINDOW_EXTRA_HEIGHT;

  board_y_offset = font_height;

  // Initialize global strings
  lstrcpy (szAppName, appname);

  // save name of Chesscmp game
  lstrcpy(szCmpFile,lpCmdLine);

  if (debug_level == 2) {
    if (debug_fptr != NULL)
      fprintf(debug_fptr,"WinMain: lpCmdLine = %s\n",lpCmdLine);
  }

  if (szCmpFile[0])
    wsprintf(szTitle,"%s - %s",szAppName,
      trim_name(szCmpFile));
  else
    lstrcpy(szTitle,szAppName);

  if (!hPrevInstance) {
     // Perform instance initialization:
     if (!InitApplication(hInstance)) {
        return (FALSE);
     }
  }

  // Perform application initialization:
  if (!InitInstance(hInstance, nCmdShow)) {
     return (FALSE);
  }

  // Main message loop:
  while (GetMessage(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  if (debug_fptr)
    fclose(debug_fptr);

  return (msg.wParam);
}

//
//  FUNCTION: InitApplication(HANDLE)
//
//  PURPOSE: Initializes window data and registers window class
//
//  COMMENTS:
//
//       In this function, we initialize a window class by filling out a data
//       structure of type WNDCLASS and calling RegisterClass.
//
BOOL InitApplication(HINSTANCE hInstance)
{
  WNDCLASSEX wcex;

  lf.lfHeight         = CHARACTER_HEIGHT;
  lf.lfWidth          = CHARACTER_WIDTH;
  lf.lfEscapement     =   0;
  lf.lfOrientation    =   0;
  lf.lfWeight         = 400;
  lf.lfItalic         =   0;
  lf.lfUnderline      =   0;
  lf.lfStrikeOut      =   0;
  lf.lfCharSet        =   0;
  lf.lfOutPrecision   =   1;
  lf.lfClipPrecision  =   2;
  lf.lfQuality        =   1;
  lf.lfPitchAndFamily =  49;
  lstrcpy(lf.lfFaceName,"Courier");
  hfont = CreateFontIndirect(&lf);

  // Fill in window class structure with parameters that describe
  // the main window.
  wcex.cbSize = sizeof(WNDCLASSEX);

  wcex.style         = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc   = (WNDPROC)WndProc;
  wcex.cbClsExtra    = 0;
  wcex.cbWndExtra    = 0;
  wcex.hInstance     = hInstance;
  wcex.hIcon         = LoadIcon(hInstance,(LPCTSTR)IDI_CHESSCMP);
  wcex.hCursor       = LoadCursor(NULL, IDC_ARROW);
  wcex.hbrBackground = CreateSolidBrush(bk_color);
  wcex.lpszMenuName  = szAppName;
  wcex.lpszClassName = szAppName;
  wcex.hIconSm       = LoadIcon(hInstance,(LPCTSTR)IDI_SMALL);

  // Register the window class and return success/failure code.
  return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HANDLE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable
//        and create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
  HWND hWnd;
  char *cpt;
  int debug_x_offset;
  int debug_y_offset;

  cpt = getenv("DEBUG_X_OFFSET");

  if (cpt != NULL)
    debug_x_offset = atoi(cpt);
  else
    debug_x_offset = 0;

  cpt = getenv("DEBUG_Y_OFFSET");

  if (cpt != NULL)
    debug_y_offset = atoi(cpt);
  else
    debug_y_offset = 0;

  hInst = hInstance; // Store instance handle in our global variable

  chesscmp_window_width = 2 * (board_x_offset + BOARD_WIDTH) + window_extra_width;
  chesscmp_window_height = board_y_offset + BOARD_HEIGHT + window_extra_height +
    bottom_margin;

  SystemParametersInfo(SPI_GETWORKAREA,0,&spi_rect,0);

  initial_x_pos = debug_x_offset + ((spi_rect.right - spi_rect.left) - chesscmp_window_width) / 2;
  initial_y_pos = debug_y_offset + ((spi_rect.bottom - spi_rect.top) - chesscmp_window_height) / 2;

  hWnd = CreateWindow(szAppName, szTitle, WS_OVERLAPPEDWINDOW,
     initial_x_pos, initial_y_pos,
     chesscmp_window_width,chesscmp_window_height,
     NULL, NULL, hInstance, NULL);

  if (!hWnd)
     return FALSE;

  ShowWindow(hWnd, nCmdShow);
  UpdateWindow(hWnd);

  return (TRUE);
}

int get_piece_offset(int piece,int rank,int file)
{
  int bBlack;
  int offset;
  int retval;

  if (debug_level == 2) {
    if (debug_fptr != NULL) {
      fprintf(debug_fptr,"dbg1 rank = %d, file = %d, piece = %2d, ",
        rank,file,piece);
    }
  }

  if (!piece) {
    /* this is a blank square; these are represented in the bitmap as well
       as pieces
    */
    if (!((rank + file) % 2))
      retval = 24;
    else
      retval = 25;
  }
  else {
    if (piece < 0) {
      bBlack = TRUE;
      piece *= -1;
    }
    else
      bBlack = FALSE;

    if ((piece >= 1) && (piece <= 6)) {
      piece--;
      offset = piece * 4;

      if (bBlack)
        offset += 2;

      if ((rank + file) % 2)
        offset++;

      retval = offset;
    }
    else
      retval = -1;
  }

  if (debug_level == 2) {
    if (debug_fptr != NULL)
      fprintf(debug_fptr,"retval = %2d\n",retval);
  }

  return retval;
}

void invalidate_rect(HWND hWnd,int rank,int file)
{
  RECT rect;

  rect.left = board_x_offset + file * width_in_pixels;
  rect.top = board_y_offset + rank * height_in_pixels;
  rect.right = rect.left + width_in_pixels;
  rect.bottom = rect.top + height_in_pixels;

  if (debug_fptr) {
    fprintf(debug_fptr,"invalidate_rect(): left = %d, top = %d, right = %d, bottom = %d\n",
      rect.left,rect.top,rect.right,rect.bottom);
  }

  InvalidateRect(hWnd,&rect,FALSE);
}

void invalidate_square(HWND hWnd,int square)
{
  int rank;
  int file;

  rank = RANK_OF(square);
  file = FILE_OF(square);

  if (!comparisons[curr_comparison].orientation)
    rank = (NUM_RANKS - 1) - rank;
  else
    file = (NUM_FILES - 1) - file;

  if (debug_fptr) {
    fprintf(debug_fptr,"invalidate_square(): rank = %d, file = %d\n",
      rank,file);
  }

  invalidate_rect(hWnd,rank,file);
}

void invalidate_board(HWND hWnd)
{
  RECT rect;

  rect.left = board_x_offset;
  rect.top = board_y_offset;
  rect.right = rect.left + NUM_FILES * width_in_pixels;
  rect.bottom = rect.top + NUM_RANKS * height_in_pixels;

  InvalidateRect(hWnd,&rect,FALSE);
}

void invalidate_board_and_coords(HWND hWnd)
{
  RECT rect;

  rect.left = 0;
  rect.top = board_y_offset;
  rect.right = rect.left + board_x_offset + NUM_FILES * width_in_pixels;
  rect.bottom = rect.top + NUM_RANKS * height_in_pixels + bottom_margin;

  InvalidateRect(hWnd,&rect,TRUE);
}

void do_paint(HWND hWnd)
{
  int m;
  int n;
  int piece;
  int piece_offset;
  int bigbmp_column;
  int bigbmp_row;
  HDC hdc;
  PAINTSTRUCT ps;
  RECT rect;
  int bSetBkColor;
  int bSelectedFont;
  char buf[80];
  int dbg;

  if (debug_fptr) {
    fprintf(debug_fptr,"top of do_paint\n");
  }

  hdc = BeginPaint(hWnd,&ps);

  for (m = 0; m < NUM_RANKS; m++) {
    for (n = 0; n < NUM_FILES; n++) {
      rect.left = board_x_offset + n * width_in_pixels;
      rect.top = board_y_offset + m * height_in_pixels;
      rect.right = rect.left + width_in_pixels;
      rect.bottom = rect.top + height_in_pixels;

      if (!RectVisible(hdc,&rect))
        continue;

      if (debug_fptr) {
        fprintf(debug_fptr,"do_paint: m = %d, n = %d\n",m,n);
      }

      if (!comparisons[curr_comparison].orientation)
        piece = get_piece2(comparisons[curr_comparison].board[0],(NUM_RANKS - 1) - m,n);
      else
        piece = get_piece2(comparisons[curr_comparison].board[0],m,(NUM_FILES - 1) - n);

      piece_offset = get_piece_offset(piece,m,n);

      if (piece_offset >= 0) {
        bigbmp_column = piece_offset;

        if ((m == highlight_rank) && (n == highlight_file))
          bigbmp_row = 2;
        else
          bigbmp_row = 0;

        if (debug_fptr && (debug_level == 2))
          fprintf(debug_fptr,"  bigbmp_column = %d, bigbmp_row = %d\n",bigbmp_column,bigbmp_row);

        BitBlt(hdc,rect.left,rect.top,
          width_in_pixels,height_in_pixels,
          hdc_compatible,
          bigbmp_column * width_in_pixels,
          bigbmp_row * height_in_pixels,
          SRCCOPY);
      }
    }
  }

  for (m = 0; m < NUM_RANKS; m++) {
    for (n = 0; n < NUM_FILES; n++) {
      rect.left = board_x_offset + 8 * width_in_pixels + board_x_offset + n * width_in_pixels;
      rect.top = board_y_offset + m * height_in_pixels;
      rect.right = rect.left + width_in_pixels;
      rect.bottom = rect.top + height_in_pixels;

      if (!RectVisible(hdc,&rect))
        continue;

      if (debug_fptr) {
        fprintf(debug_fptr,"do_paint: m = %d, n = %d\n",m,n);
      }

      if (!comparisons[curr_comparison].orientation)
        piece = get_piece2(comparisons[curr_comparison].board[1],(NUM_RANKS - 1) - m,n);
      else
        piece = get_piece2(comparisons[curr_comparison].board[1],m,(NUM_FILES - 1) - n);

      piece_offset = get_piece_offset(piece,m,n);

      if (piece_offset >= 0) {
        bigbmp_column = piece_offset;

        if ((m == highlight_rank) && (n == highlight_file))
          bigbmp_row = 2;
        else
          bigbmp_row = 0;

        if (debug_fptr && (debug_level == 2))
          fprintf(debug_fptr,"  bigbmp_column = %d, bigbmp_row = %d\n",bigbmp_column,bigbmp_row);

        BitBlt(hdc,rect.left,rect.top,
          width_in_pixels,height_in_pixels,
          hdc_compatible,
          bigbmp_column * width_in_pixels,
          bigbmp_row * height_in_pixels,
          SRCCOPY);
      }
    }
  }

  bSetBkColor = FALSE;
  bSelectedFont = FALSE;

  rect.left = 0;
  rect.top = 0;
  rect.right = chesscmp_window_width;
  rect.bottom = rect.top + 16;

  if (RectVisible(hdc,&rect)) {
    if (!bSetBkColor && (bk_color != COLOR_WHITE)) {
      SetBkColor(hdc,bk_color);
      bSetBkColor = TRUE;
    }

    if (!bSelectedFont) {
      SelectObject(hdc,hfont);
      bSelectedFont = TRUE;
    }
  }

  rect.top = 16;
  rect.right = chesscmp_window_width;
  rect.bottom = rect.top + 16;

  if (RectVisible(hdc,&rect)) {
    if (!bSetBkColor && (bk_color != COLOR_WHITE)) {
      SetBkColor(hdc,bk_color);
      bSetBkColor = TRUE;
    }

    if (!bSelectedFont) {
      SelectObject(hdc,hfont);
      bSelectedFont = TRUE;
    }
  }

  // display the ranks, if necessary
  rect.left = 2;
  rect.right = rect.left + CHARACTER_WIDTH;

  for (m = 0; m < NUM_RANKS; m++) {
    rect.top = board_y_offset + m * height_in_pixels + 19;
    rect.bottom = rect.top + CHARACTER_HEIGHT;

    if (RectVisible(hdc,&rect)) {
      if (!bSetBkColor && (bk_color != COLOR_WHITE)) {
        SetBkColor(hdc,bk_color);
        bSetBkColor = TRUE;
      }

      if (!bSelectedFont) {
        SelectObject(hdc,hfont);
        bSelectedFont = TRUE;
      }

      if (!comparisons[curr_comparison].orientation)
        buf[0] = '1' + (NUM_RANKS - 1) - m;
      else
        buf[0] = '1' + m;

      TextOut(hdc,rect.left,rect.top,buf,1);
    }
  }

  rect.left = board_x_offset + 8 * width_in_pixels + 2;
  rect.right = rect.left + CHARACTER_WIDTH;

  for (m = 0; m < NUM_RANKS; m++) {
    rect.top = board_y_offset + m * height_in_pixels + 19;
    rect.bottom = rect.top + CHARACTER_HEIGHT;

    if (RectVisible(hdc,&rect)) {
      if (!bSetBkColor && (bk_color != COLOR_WHITE)) {
        SetBkColor(hdc,bk_color);
        bSetBkColor = TRUE;
      }

      if (!bSelectedFont) {
        SelectObject(hdc,hfont);
        bSelectedFont = TRUE;
      }

      if (!comparisons[curr_comparison].orientation)
        buf[0] = '1' + (NUM_RANKS - 1) - m;
      else
        buf[0] = '1' + m;

      TextOut(hdc,rect.left,rect.top,buf,1);
    }
  }

  // display the files, if necessary
  rect.top = board_y_offset + NUM_RANKS * height_in_pixels + 2;
  rect.bottom = rect.top + CHARACTER_HEIGHT;

  for (m = 0; m < NUM_FILES; m++) {
    rect.left = board_x_offset + m * width_in_pixels + 21;
    rect.right = rect.left + CHARACTER_WIDTH;

    if (RectVisible(hdc,&rect)) {
      if (!bSetBkColor && (bk_color != COLOR_WHITE)) {
        SetBkColor(hdc,bk_color);
        bSetBkColor = TRUE;
      }

      if (!bSelectedFont) {
        SelectObject(hdc,hfont);
        bSelectedFont = TRUE;
      }

      if (!comparisons[curr_comparison].orientation)
        buf[0] = 'a' + m;
      else
        buf[0] = 'a' + (NUM_FILES - 1) - m;

      TextOut(hdc,rect.left,rect.top,buf,1);
    }
  }

  for (m = 0; m < NUM_FILES; m++) {
    rect.left = board_x_offset + 8 * width_in_pixels + board_x_offset + m * width_in_pixels + 21;
    rect.right = rect.left + CHARACTER_WIDTH;

    if (RectVisible(hdc,&rect)) {
      if (!bSetBkColor && (bk_color != COLOR_WHITE)) {
        SetBkColor(hdc,bk_color);
        bSetBkColor = TRUE;
      }

      if (!bSelectedFont) {
        SelectObject(hdc,hfont);
        bSelectedFont = TRUE;
      }

      if (!comparisons[curr_comparison].orientation)
        buf[0] = 'a' + m;
      else
        buf[0] = 'a' + (NUM_FILES - 1) - m;

      TextOut(hdc,rect.left,rect.top,buf,1);
    }
  }

  EndPaint(hWnd,&ps);
}

static int click_in_board(int x,int y)
{
  if ((x >= board_x_offset) && (x < board_x_offset + BOARD_WIDTH) &&
      (y >= board_y_offset) && (y < board_y_offset + BOARD_HEIGHT))
    return TRUE;

  return FALSE;
}

static int board_square(int x,int y,int *board_rank_pt,int *board_file_pt)
{
  if (!click_in_board(x,y))
    return FALSE;

  x -= board_x_offset;
  y -= board_y_offset;

  *board_rank_pt = y / height_in_pixels;
  *board_file_pt = x / width_in_pixels;

  return TRUE;
}

static void handle_char_input(HWND hWnd,WPARAM wParam)
{
  if ((wParam == 'e') || (wParam == 'E'))
    DestroyWindow(hWnd);
}

void do_read(HWND hWnd,LPSTR name,int *num_comparisons_pt,struct board_comparison **comparisons_pt)
{
  int retval;
  char buf[256];

  retval = read_board_comparisons(name,num_comparisons_pt,comparisons_pt);

  if (!retval) {
    wsprintf(szTitle,"%s - %s",szAppName,
      trim_name(name));
    SetWindowText(hWnd,szTitle);
    highlight_rank = -1;
    highlight_file = -1;
    InvalidateRect(hWnd,NULL,TRUE);
  }
  else {
    wsprintf(buf,read_board_comparisons_failure,name,retval);
    MessageBox(hWnd,buf,NULL,MB_OK);
  }
}

//
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
//
//  MESSAGES:
//
// WM_COMMAND - process the application menu
// WM_PAINT - Paint the main window
// WM_DESTROY - post a quit message and return
//    WM_DISPLAYCHANGE - message sent to Plug & Play systems when the display
//                       changes
//    WM_RBUTTONDOWN - Right mouse click -- put up context menu here if
//                     appropriate
//    WM_NCRBUTTONUP - User has clicked the right button on the application's
//                     system menu
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  int n;
  int wmId, wmEvent;
  int loword;
  int hiword;
  int file;
  int rank;
  int retval;
  LPSTR name;
  HDC hdc;
  RECT rect;

  switch (message) {
    case WM_CREATE:
      // load the Chesscmp piece bitmap
      chesscmp_piece_bitmap_handle = LoadBitmap(
        hInst,chesscmp_piece_bitmap_name);

      if (chesscmp_piece_bitmap_handle != NULL) {
        hdc_compatible = CreateCompatibleDC(GetDC(hWnd));
        SelectObject(hdc_compatible,chesscmp_piece_bitmap_handle);
      }

      // initialize the structure used for opening a file
      OpenFileName.lStructSize       = sizeof(OPENFILENAME);
      OpenFileName.hwndOwner         = hWnd;
      OpenFileName.hInstance         = hInst;
      OpenFileName.lpstrFilter       = chesscmp_filter;
      OpenFileName.lpstrCustomFilter = NULL;
      OpenFileName.nMaxCustFilter    = 0;
      OpenFileName.nFilterIndex      = 1;
      OpenFileName.lpstrFile         = szCmpFile;
      OpenFileName.nMaxFile          = sizeof(szCmpFile);
      OpenFileName.lpstrFileTitle    = NULL;
      OpenFileName.nMaxFileTitle     = 0;
      OpenFileName.lpstrInitialDir   = NULL;
      OpenFileName.lpstrTitle        = "Open a comparison file";
      OpenFileName.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY |
        OFN_EXTENSIONDIFFERENT;
      OpenFileName.nFileOffset       = 0;
      OpenFileName.nFileExtension    = 0;
      OpenFileName.lpstrDefExt       = chesscmp_ext;
      OpenFileName.lCustData         = 0;
      OpenFileName.lpfnHook          = NULL;
      OpenFileName.lpTemplateName    = NULL;

      WriteFileName.lStructSize = sizeof(OPENFILENAME);
      WriteFileName.hwndOwner = hWnd;
      WriteFileName.hInstance = hInst;
      WriteFileName.lpstrFilter = chesscmp_filter;
      WriteFileName.lpstrCustomFilter = NULL;
      WriteFileName.nMaxCustFilter = 0;
      WriteFileName.nFilterIndex = 1;
      WriteFileName.lpstrFile = szCmpWriteFile;
      WriteFileName.nMaxFile = sizeof szCmpWriteFile;
      WriteFileName.lpstrFileTitle = NULL;
      WriteFileName.nMaxFileTitle = 0;
      WriteFileName.lpstrInitialDir = NULL;
      WriteFileName.lpstrTitle = NULL;
      WriteFileName.Flags = OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY |
        OFN_EXTENSIONDIFFERENT;
      WriteFileName.nFileOffset = 0;
      WriteFileName.nFileExtension = 0;
      WriteFileName.lpstrDefExt = chesscmp_ext;
      WriteFileName.lCustData = 0;
      WriteFileName.lpfnHook = NULL;
      WriteFileName.lpTemplateName = NULL;

      // read the game passed on the command line, if there is one
      if (szCmpFile[0])
        do_read(hWnd,szCmpFile,&num_comparisons,&comparisons);

      highlight_rank = -1;
      highlight_file = -1;

      InvalidateRect(hWnd,NULL,TRUE);

      break;

    case WM_SIZE:
      // Tell the toolbar to resize itself to fill the top of the window.
      SendMessage(hWndToolBar, TB_AUTOSIZE, 0L, 0L);
      break;

    case WM_CHAR:
      handle_char_input(hWnd,wParam);

      break;

    case WM_KEYDOWN:
      switch (wParam) {
        case VK_F2:
          curr_comparison++;

          if (curr_comparison == num_comparisons)
            curr_comparison = 0;

          InvalidateRect(hWnd,NULL,TRUE);

          break;

        case VK_F3:
          curr_comparison--;

          if (curr_comparison == -1)
            curr_comparison = num_comparisons - 1;

          InvalidateRect(hWnd,NULL,TRUE);

          break;
      }

      break;

    case WM_COMMAND:
      wmId    = LOWORD(wParam); // Remember, these are...
      wmEvent = HIWORD(wParam); // ...different for Win32!

      //Parse the menu selections:
      switch (wmId) {
        case IDM_OPEN:
          // Call the common dialog function.
          if (GetOpenFileName(&OpenFileName)) {
            name = OpenFileName.lpstrFile;
            do_read(hWnd,name,&num_comparisons,&comparisons);
          }

          break;

        case IDM_ABOUT:
           DialogBox(hInst,"AboutBox",hWnd,(DLGPROC)About);

           break;

        case IDM_EXIT:
          DestroyWindow (hWnd);
          break;

        default:
          return (DefWindowProc(hWnd, message, wParam, lParam));
      }

      break;

    case WM_LBUTTONDOWN:
      loword = LOWORD(lParam);
      hiword = HIWORD(lParam);

      if ((loword >= board_x_offset) && (hiword >= board_y_offset)) {
        file = (loword - board_x_offset) / width_in_pixels;
        rank = (hiword - board_y_offset) / height_in_pixels;
        do_lbuttondown(hWnd,file,rank);
      }

      break;

    case WM_PAINT:
      do_paint(hWnd);

      break;

    case WM_DESTROY:
      if (chesscmp_piece_bitmap_handle != NULL)
        DeleteDC(hdc_compatible);

      PostQuitMessage(0);

      break;

    default:
      return (DefWindowProc(hWnd, message, wParam, lParam));
  }

  return (0);
}

char *trim_name(char *name)
{
  int n;

  for (n = strlen(name) - 1; (n >= 0); n--)
    if (name[n] == '\\')
      break;

  n++;

  return &name[n];
}

//
//  FUNCTION: About(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for "About" dialog box
//
//  MESSAGES:
//
// WM_INITDIALOG - initialize dialog box
// WM_COMMAND    - Input received
//
//
LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
   switch (message) {
        case WM_INITDIALOG:
         ShowWindow (hDlg, SW_HIDE);

         CenterWindow (hDlg, GetWindow (hDlg, GW_OWNER));

         ShowWindow (hDlg, SW_SHOW);

         return (TRUE);

      case WM_COMMAND:
         if (LOWORD(wParam) == IDOK) {
            EndDialog(hDlg, TRUE);
            return (TRUE);
         }
         else if (LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, FALSE);
            return (FALSE);
         }

         break;
   }

    return FALSE;
}

//   FUNCTION: CenterWindow(HWND, HWND)
//
//   PURPOSE: Centers one window over another.
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
//       This functionwill center one window over another ensuring that
//    the placement of the window is within the 'working area', meaning
//    that it is both within the display limits of the screen, and not
//    obscured by the tray or other framing elements of the desktop.
BOOL CenterWindow (HWND hwndChild, HWND hwndParent)
{
   RECT    rChild, rParent, rWorkArea;
   int     wChild, hChild, wParent, hParent;
   int     xNew, yNew;
   BOOL  bResult;

   // Get the Height and Width of the child window
   GetWindowRect (hwndChild, &rChild);
   wChild = rChild.right - rChild.left;
   hChild = rChild.bottom - rChild.top;

   // Get the Height and Width of the parent window
   GetWindowRect (hwndParent, &rParent);
   wParent = rParent.right - rParent.left;
   hParent = rParent.bottom - rParent.top;

   // Get the limits of the 'workarea'
   bResult = SystemParametersInfo(
      SPI_GETWORKAREA,  // system parameter to query or set
      sizeof(RECT),
      &rWorkArea,
      0);
   if (!bResult) {
      rWorkArea.left = rWorkArea.top = 0;
      rWorkArea.right = GetSystemMetrics(SM_CXSCREEN);
      rWorkArea.bottom = GetSystemMetrics(SM_CYSCREEN);
   }

   // Calculate new X comparison, then adjust for workarea
   xNew = rParent.left + ((wParent - wChild) /2);
   if (xNew < rWorkArea.left) {
      xNew = rWorkArea.left;
   } else if ((xNew+wChild) > rWorkArea.right) {
      xNew = rWorkArea.right - wChild;
   }

   // Calculate new Y comparison, then adjust for workarea
   yNew = rParent.top  + ((hParent - hChild) /2);
   if (yNew < rWorkArea.top) {
      yNew = rWorkArea.top;
   } else if ((yNew+hChild) > rWorkArea.bottom) {
      yNew = rWorkArea.bottom - hChild;
   }

   // Set it, and return
   return SetWindowPos (hwndChild, NULL, xNew, yNew, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

void do_lbuttondown(HWND hWnd,int file,int rank)
{
  if (debug_fptr != NULL) {
    fprintf(debug_fptr,"do_lbuttondown: rank = %d, file = %d\n",rank,file);
  }
}
