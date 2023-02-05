/*============================================================================
 *  chardev/gfxcondev.c
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

/*============================================================================
 * ==========================================================================*/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <pico/stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <bearos/devctl.h>
#include <bearos/terminal.h>
#include <string.h>
#include <sys/error.h>
#include <errno.h>
#include <ctype.h>
#include <fatfs_loopback/fatloopback.h>
#include <chardev/gfxcondev.h>
#include <gfx/fonts.h>
#include <gfx/gfx_util.h>

// Modes of the character parser -- used when parsing esc sequences
// MODE_NORMAL -- initials state, or unknown
#define MODE_NORMAL 0
// MODE_ESC -- I have received an ESC, and now I must determine whether it
//   is the start of a CSI or something else
#define MODE_ESC 1
// I have received ESC-[, and must wait for a character that is not a number
//   or ;
#define MODE_CSI 2

#define CURSOR_HEIGHT 1 

// Maximum length of a CSI escape sequence, not including the esc-[ header
#define MAX_CSI 10

struct _GfxConDev
  {
  char *name;
  // My device descriptor
  DevDescriptor *desc;
  // Dev descriptor of the underlying gfx device
  DevDescriptor *gfxdevdesc;
  // File descriptor of the underlying gfx device
  FileDesc *gfx_file_desc;
  sFONT *font;
  int font_width;
  int font_height;
  // row_height is the height of the font plus space for the cursor
  int row_height;
  int32_t flags;
  int bytes_pp;
  void *glyph_buffer;
  int row;
  int col;
  // temp_rows, temp_cols, etc are the text height and width for a specific 
  //   write operation. We don't cache these values in the long term, because
  //   the hardware properties could change. However, they are unlikely to
  //   change during a single write(), so we can cache them for at least that
  //   time.
  int temp_rows;
  int temp_cols;
  int temp_pixel_width;
  int tab_size;
  int mode; // One of the MODE_XXX constants
  int csilen;
  char csibuf[MAX_CSI + 1];
  };

static void gfxcon_write_printing_char (FileDesc *f, uint8_t c); // FWD

/*============================================================================
 * gfxcondev_get_glyph_offset
 * Returns the pixel x and y of the TL corner of the current glyph, that is,
 *   at the position self->row,self->col
* ==========================================================================*/
#if PICO_ON_DEVICE
static void gfxcondev_get_glyph_offset (GfxConDev *self, int *x, int *y)
  {
  *x = self->col * self->font_width;
  *y = self->row * self->row_height;
  }
#endif

/*============================================================================
 * gfxcondev_get_text_size
* ==========================================================================*/
static void gfxcondev_get_text_size (GfxConDev *self, int *rows, int *cols,
              int *pixel_width)
  {
#if PICO_ON_DEVICE
  DevCtlGfxProps gfx_props;
  self->gfx_file_desc->devctl 
        (self->gfx_file_desc, DC_GFX_GET_PROPS, (int32_t)&gfx_props);      

  *pixel_width = gfx_props.width;
  *rows = gfx_props.height / self->row_height;
  *cols = gfx_props.width / self->font_width; 
#else
  *rows = 24; 
  *cols = 80;
  (void)self; (void)pixel_width;
#endif
  }

/*============================================================================
 * gfxcondev_read
* ==========================================================================*/
int gfxcon_read (FileDesc *f, void *buffer, int len)
  {
  // TODO
  return 0;
  }

/*============================================================================
 * gfxcondev_read_timeout
* ==========================================================================*/
int gfxcon_read_timeout (FileDesc *f, int msec)
  {
  GfxConDev *self = f->self;
  
  return -1;
  }

/*============================================================================
 * gfxcon_font_get_offset
* ==========================================================================*/
const uint8_t *gfxcon_font_get_offset (int c, const sFONT *font)
  {
  if (c < ' ') return NULL;
  if (c > 126) return NULL; 
  return font->table + ((c - 32) * (font->Bytes * font->Height));
  return 0;
  }

/*============================================================================
 * gfxcon_fill_glyph_buffer
* ==========================================================================*/
#if PICO_ON_DEVICE
static void gfxcon_fill_glyph_buffer (FileDesc *f, uint8_t c)
  {
  GfxConDev *self = f->self;
  const uint32_t white = 0xFFFFFFFF; // TODO real colour
  int bytes_pp = self->bytes_pp;

  int font_height = self->font_height;
  int font_width = self->font_width;

  memset (self->glyph_buffer, 0, (size_t)(bytes_pp * 
     self->font_width * self->font_height)); // TODO background colour

  const uint8_t *offset = gfxcon_font_get_offset (c, self->font); 
  if (!offset)
    offset = gfxcon_font_get_offset ('?', self->font); 
  int font_width_bytes = self->font->Bytes;

  for (int i = 0; i < font_height; i++)
    {
    const uint8_t *cur_offset = offset + i * font_width_bytes; 
    uint8_t cur_byte = *cur_offset;
    
    register int cur_bit_p = 0;
    for (int j = 0; j < font_width; j++)
      {
      if (cur_byte & 0x80)
        {
        memcpy (self->glyph_buffer + (i * font_width + j) 
          * self->bytes_pp, &white, (size_t)bytes_pp); 
        }
      cur_byte <<= 1;
      cur_bit_p++;
      if (cur_bit_p == 8)
        {
        cur_offset++;
        cur_bit_p = 0; 
        cur_byte = *cur_offset;
        } 
      } 
    } 
  }
#endif

/*============================================================================
 * gfxcon_write_cr
* ==========================================================================*/
static void gfxcon_write_cr (FileDesc *f)
  {
  GfxConDev *self = f->self;
  self->col = 0;
  }

/*============================================================================
 * gfxcon_write_lf
* ==========================================================================*/
static void gfxcon_write_lf (FileDesc *f)
  {
  GfxConDev *self = f->self;
  self->row++;
  if (self->row == self->temp_rows) 
    {
    self->gfx_file_desc->devctl 
        (self->gfx_file_desc, DC_GFX_SCROLL_UP, self->row_height); 
    self->row = self->temp_rows - 1;

    DevCtlGfxRegion region;
    region.x = 0; 
    region.y = self->row_height * (self->temp_rows - 1); 
    region.cx = self->temp_pixel_width; 
    region.cy = 480 - region.y; 
    self->gfx_file_desc->devctl (self->gfx_file_desc, DC_GFX_SET_REGION, 
      (intptr_t)&region);
    int32_t black = 0x0; // TODO -- use real background colour
    self->gfx_file_desc->devctl (self->gfx_file_desc, DC_GFX_FILL, 
      (intptr_t)&black);

    }
  }

/*============================================================================
 * gfxcon_write_newline
 * write_newline writes a real newline, that is, a CR followed by a LF.
 * In some set-ups, a CR will generate a newline (that is, imply an LF).
* ==========================================================================*/
static void gfxcon_write_newline (FileDesc *f)
  {
  gfxcon_write_cr (f);
  gfxcon_write_lf (f);
  }

/*============================================================================
 * gfxcon_write_backspace_nondestructive
* ==========================================================================*/
static void gfxcon_write_backspace_nondestructive (FileDesc *f)
  {
  gfxcondev_cursor_back (f->self, 1);
  }

/*============================================================================
 * gfxcon_write_backspace_destructive
* ==========================================================================*/
static void gfxcon_write_backspace_destructive (FileDesc *f)
  {
  GfxConDev *self = f->self;
  gfxcon_write_backspace_nondestructive (f);
  gfxcon_write_printing_char (f, ' ');
  gfxcon_write_backspace_nondestructive (f);
  }

/*============================================================================
 * gfxcon_write_backspace
* ==========================================================================*/
static void gfxcon_write_backspace (FileDesc *f)
  {
  GfxConDev *self = f->self;
  if (self->flags & DC_TERM_FLAG_BS_DESTRUCTIVE)
    gfxcon_write_backspace_destructive (f);
  else
    gfxcon_write_backspace_nondestructive (f);
  }

/*============================================================================
 * gfxcon_write_tab
* ==========================================================================*/
static void gfxcon_write_tab (FileDesc *f)
  {
  GfxConDev *self = f->self;
  int tabs = self->col / self->tab_size; 
  tabs++;
  self->col += (tabs * self->tab_size);

  if ((self->flags & DC_TERM_FLAG_WRAP) && (self->col >= self->temp_cols))
    {
    gfxcon_write_newline (f);
    self->col = self->tab_size; 
    }
  }


/*============================================================================
 * gfxcon_write_printing_char
* ==========================================================================*/
static void gfxcon_write_printing_char (FileDesc *f, uint8_t c)
  {
  GfxConDev *self = f->self;
  int x, y;
  if (self->col < self->temp_cols)
    {
#if PICO_ON_DEVICE
    gfxcondev_get_glyph_offset (f->self, &x, &y);
    DevCtlGfxRegion region;
    region.x = x; 
    region.y = y; 
    region.cx = self->font_width; 
    region.cy = self->font_height; 
    self->gfx_file_desc->devctl (self->gfx_file_desc, DC_GFX_SET_REGION, 
      (int32_t)&region);
    gfxcon_fill_glyph_buffer (f, c); 
    self->gfx_file_desc->write (self->gfx_file_desc, self->glyph_buffer, 
      self->bytes_pp * self->font_width * self->font_height);
#else
    (void)x; (void)y;
    putchar (c);
#endif
    }
  self->col++;
  if ((self->flags & DC_TERM_FLAG_WRAP) && (self->col >= self->temp_cols))
    {
    gfxcon_write_newline (f);
    }
  }

/*============================================================================
 * gfxcon_csi
 * // Note -- this function will modify the string passed to it
* ==========================================================================*/
static void gfxcon_csi (FileDesc *f, char *csi)
  {
  GfxConDev *self = f->self;

  int l = (int)strlen (csi);
  if (l == 0) return;
  char terminator = csi[l - 1];
  csi[l - 1] = 0;
 
  // At present, no CSI sequence we handle has more than three arguments,
  //   and all are numbers.
  int arg[3];
  int argc = 0; 

  // The way we split the CSI sequence on semicolons might need work
  char numbuff[5];
  int numind = 0; 
  while (*csi)
    {
    char c = *csi;
    if (c == ';')
      {
      numbuff[numind] = 0;
      if (argc < 3)
        {
        arg[argc] = atoi (numbuff);
        argc++;
        numind = 0;
        numbuff[0] = 0;
        }
      }
    else
      {
      if (numind < (int)sizeof (numbuff) - 1)
        numbuff[numind++] = c;
      }   
    csi++;
    }

  if (numind && (argc < 3))
    {
    arg[argc] = atoi (numbuff);
    argc++;
    }

  /*
  printf ("terminator=%c\n", terminator); // TODO
  for (int i = 0; i < argc; i++) // TODO
    {
    printf ("%d: %d\n", i, arg[i]);
    }
  */

  //printf ("arg0=%d arg1=%d\n", arg[0], arg[1]);

  switch (terminator)
    {
    case 'A':
      if (argc == 0)
        gfxcondev_cursor_up (self, 1); 
      else if (argc == 2)
        {
        if (arg[0] == 0) arg[0] = 1;
        gfxcondev_cursor_up (self, arg[0]); 
        }
      break;
    case 'B':
      if (argc == 0)
        gfxcondev_cursor_down (self, 1); 
      else if (argc == 2)
        {
        if (arg[0] == 0) arg[0] = 1;
        gfxcondev_cursor_down (self, arg[0]); 
        }
      break;
    case 'C':
      if (argc == 0)
        gfxcondev_cursor_forward (self, 1); 
      else if (argc == 2)
        {
        if (arg[0] == 0) arg[0] = 1;
        gfxcondev_cursor_forward (self, arg[0]); 
        }
      break;
    case 'D':
      if (argc == 0)
        gfxcondev_cursor_back (self, 1); 
      else if (argc == 2)
        {
        if (arg[0] == 0) arg[0] = 1;
        gfxcondev_cursor_back (self, arg[0]); 
        }
      break;
    case 'H':
      if (argc == 0)
        gfxcondev_set_cursor (self, 0, 0); 
      else if (argc == 2)
        {
        if (arg[0] == 0) arg[0] = 1;
        if (arg[1] == 0) arg[1] = 1;
        gfxcondev_set_cursor (self, arg[0] - 1, arg[1] - 1); 
        }
      break;
    case 'J':
      if (argc == 0)
        gfxcondev_clear_screen (self, 2); 
      else if (argc == 1)
        {
        gfxcondev_clear_screen (self, arg[0]); 
        }
      break;
    }
  }

/*============================================================================
 * gfxcon_write_char
* ==========================================================================*/
static void gfxcon_write_char (FileDesc *f, uint8_t c)
  {
  GfxConDev *self = f->self;
  if (self->mode == MODE_CSI)
    {
    if (self->csilen < MAX_CSI)
      {
      self->csibuf[self->csilen] = (char)c;
      self->csilen++;
      }
    if (isdigit (c) || c == ';')
      {
      }
    else
      {
      if (self->csilen > 0)
        {
        self->csibuf [self->csilen] = 0;
        gfxcon_csi (f, self->csibuf); 
        self->csilen = 0;
        self->mode = MODE_NORMAL;
        }
      }
    }
  else if (self->mode == MODE_ESC)
    {
    switch (c)
      {
      case 'c': // RESET
        gfxcondev_full_reset (self);
        self->mode = MODE_NORMAL;
        break;
      case '[': // enter CSI mode 
        self->mode = MODE_CSI;
        break;
      default:
        self->mode = MODE_NORMAL; // Generally, just ignore the char
                                   //   after the escape
      }
    }
  else 
    { // MODE_NORMAL
    switch (c)
      {
      case 8:
	gfxcon_write_backspace (f);
	break;     
   
      case 9:
	gfxcon_write_tab (f);
	break;     
   
      case 10:
	gfxcon_write_lf (f); 
	if (self->flags & DC_TERM_FLAG_LFISCRLF)
	  gfxcon_write_cr (f); 
	break; 

      case 13:
	gfxcon_write_cr (f); 
	break; 

      case 27:
        self->mode = MODE_ESC; 
        break;

      case 127:
	gfxcon_write_backspace_destructive (f);
        break;

      default: gfxcon_write_printing_char (f, c);
      }
    }
  }

/*============================================================================
 * gfxcon_write
* ==========================================================================*/
int gfxcon_write (FileDesc *f, const void *buffer, int len)
  {
  GfxConDev *self = f->self;
  gfxcondev_cursor_show (self, false);
  for (int i = 0; i < len; i++)
    {
    gfxcon_write_char (f, ((uint8_t *)buffer)[i]);
    }
  gfxcondev_cursor_show (self, true); // TODO -- cursor may be hidden
  return len;
  }

/*============================================================================
 * gfxcon_close 
* ==========================================================================*/
Error gfxcon_close (FileDesc *f)
  {
  free (f);
  return 0;
  }

/*============================================================================
 * gfxcondev_gfxconalloc_buffers
 gfxcon* On entry, font_width and font_height must have been set already
* ==========================================================================*/
void gfxcondev_alloc_buffers (GfxConDev *self)
  {
  if (self->glyph_buffer) free (self->glyph_buffer);

#if PICO_ON_DEVICE
  DevCtlGfxProps gfx_props;
  self->gfx_file_desc->devctl 
        (self->gfx_file_desc, DC_GFX_GET_PROPS, (int32_t)&gfx_props);      
  self->bytes_pp = gfx_util_get_bytes_pp (gfx_props.colmode);
#else
  self->bytes_pp = 2;
#endif

  size_t glyph_buffer_size = (size_t)(self->font_width * 
          self->font_height * self->bytes_pp);
  self->glyph_buffer = malloc (glyph_buffer_size);
  }

/*============================================================================
 * gfxcon_devctl 
* ==========================================================================*/
Error gfxcon_devctl (FileDesc *f, intptr_t arg1, intptr_t arg2)
  {
  GfxConDev *self = f->self;

  switch (arg1)
    {
    case DC_TERM_SET_FLAGS:
      self->flags = (int32_t)arg2;
      return 0;
    case DC_TERM_GET_FLAGS:
      *((int32_t *)arg2) = self->flags;
      return 0;
    case DC_TERM_GET_PROPS:
      int rows, cols, dummy;
      gfxcondev_get_text_size (self, &rows, &cols, &dummy);
      DevCtlTermProps *props = (DevCtlTermProps*)arg2;
      props->rows = rows; 
      props->cols = cols; 
      return 0;
    case DC_GET_GEN_FLAGS:
      *((int32_t *)arg2) = DC_FLAG_ISTTY;
      return 0;
    // DEBUG_ONLY!
    case 99:
       gfxcondev_adjust_to_hardware (self);
    } 
  return EINVAL;
  }

/*============================================================================
 * gfxcondev_set_cursor
* ==========================================================================*/
void gfxcondev_set_cursor (GfxConDev *self, int row, int col)
  {
  if (row < 0) row = 0;
  if (col < 0) col = 0;
  if (row >= self->temp_rows) row = self->temp_rows - 1;
  if (col >= self->temp_cols) col = self->temp_cols - 1;
  self->row = row;
  self->col = col;
#if PICO_ON_DEVICE
#else
  printf ("gfxcondev set cursor r=%d c=%d\n", row, col);
#endif
  // TODO show cursor
  }

/*============================================================================
 * gfxcondev_cursor_up
* ==========================================================================*/
void gfxcondev_cursor_up (GfxConDev *self, int rows)
  {
  if (rows <= 0) rows = 1;
  self->row -= rows;
  if (self->row < 0) self->row = 0;
  }

/*============================================================================
 * gfxcondev_cursor_down
* ==========================================================================*/
void gfxcondev_cursor_down (GfxConDev *self, int rows)
  {
  if (rows <= 0) rows = 1;
  self->row += rows;
  if (self->row >= self->temp_rows) self->row = self->temp_rows - 1;
  // TODO: should we scroll if on bottom row?
  }

/*============================================================================
 * gfxcondev_cursor_back
* ==========================================================================*/
void gfxcondev_cursor_back (GfxConDev *self, int cols)
  {
  if (cols <= 0) cols = 1;
  for (int i = 0; i < cols; i++)
    {
    if (self->col > 0)
      {
      self->col--;
      }
    else
      {
      // Already on leftmost column
      if (self->flags & DC_TERM_FLAG_WRAP)
	{
	if (self->row > 0)
	  {
	  // We can back up a row
	  self->row--;
	  self->col = self->temp_cols - 1;
	  }
	}
      else
	{
	// Do nothing -- we aren't wrapping, so cursor stays on left
	}
      }
    }
  }

/*============================================================================
 * gfxcondev_cursor_forward
* ==========================================================================*/
void gfxcondev_cursor_forward (GfxConDev *self, int cols)
  {
  if (cols <= 0) cols = 1;
  self->col += cols;
  if (self->col >= self->temp_cols)
    {
    if (self->flags & DC_TERM_FLAG_WRAP)
      {
      int add_rows = self->col / self->temp_cols;
      gfxcondev_cursor_down (self, add_rows);
      self->col = self->col % self->temp_cols;
      }
    else
      {
      self->col = self->temp_cols - 1;
      // Or should we allow the cursor to run off the screen?
      }
    }
  }

/*============================================================================
 * gfxcondev_cursor_show
* ==========================================================================*/
void gfxcondev_cursor_show (const GfxConDev *self, bool visible)
  {
  uint32_t colour;
  if (visible)
    colour = 0xFFFFFFFF; // TODO user back/fore colours
  else
    colour = 0;

#if PICO_ON_DEVICE
    DevCtlGfxRegion region;
    region.x = self->col * self->font_width; 
    region.y = self->row * self->row_height + self->font_height; 
    region.cx = self->font_width; 
    region.cy = CURSOR_HEIGHT; 
    self->gfx_file_desc->devctl (self->gfx_file_desc, DC_GFX_SET_REGION, 
      (int32_t)&region);
    self->gfx_file_desc->devctl (self->gfx_file_desc, DC_GFX_FILL, 
      (int32_t)&colour);
#else
  (void)colour; (void)self;
#endif
  }

/*============================================================================
 * gfxcondev_clear_screen
 * TODO: 'what' is not yet implemented
* ==========================================================================*/
void gfxcondev_clear_screen (GfxConDev *self, int what)
  {
#if PICO_ON_DEVICE
  if (what == 2)
    {
    self->gfx_file_desc->devctl 
        (self->gfx_file_desc, DC_GFX_RESET_AND_CLEAR, 0);      
    }
#else
  printf ("gfxcondev clear screen\n");
#endif

  gfxcondev_cursor_show (self, true);
  }

/*============================================================================
 * gfxcondev_full_reset
* ==========================================================================*/
void gfxcondev_full_reset (GfxConDev *self)
  {
  self->flags = DC_TERM_FLAG_ECHO | DC_TERM_FLAG_WRAP | DC_TERM_FLAG_LFISCRLF
     | DC_TERM_FLAG_BS_NONDESTRUCTIVE;
  self->tab_size = 8;
  self->mode = 0;
  self->csilen = 0;
  gfxcondev_clear_screen (self, 2);
  //set_cursor could fail here, because it constrains the cursor pos to
  //  (temp_rows, temp_cols), which may not have been set at this point
  //gfxcondev_set_cursor (self, 0, 0);
  self->row = 0;
  self->col = 0;
  }

/*============================================================================
 * gfxcon_get_file_desc
* ==========================================================================*/
FileDesc *gfxcon_get_file_desc (DevDescriptor *dev_desc)
  {
  GfxConDev *self = dev_desc->self;
  FileDesc *f = malloc (sizeof (FileDesc));
  memset (f, 0, sizeof (FileDesc));
  //eoi = false;
  f->self = self;
  f->close = gfxcon_close;
  f->write = gfxcon_write;
  f->read = gfxcon_read;
  f->read_timeout = gfxcon_read_timeout;
  f->devctl = gfxcon_devctl;
  return f;
  }

/*============================================================================
 * gfxcondev_adjust_to_hardware
* ==========================================================================*/
void gfxcondev_adjust_to_hardware (GfxConDev *self)
  {
  self->gfx_file_desc = self->gfxdevdesc->get_file_desc (self->gfxdevdesc);

  DevCtlGfxProps gfx_props;
  self->gfx_file_desc->devctl 
        (self->gfx_file_desc, DC_GFX_GET_PROPS, (int32_t)&gfx_props);      
  self->bytes_pp = gfx_util_get_bytes_pp (gfx_props.colmode);

  // TODO -- we need a way to change font at runtime
  self->font = &Font16;
  self->font_width = self->font->Width;
  self->font_height = self->font->Height;
  self->row_height = self->font_height + CURSOR_HEIGHT;

  gfxcondev_get_text_size (self, &(self->temp_rows), &(self->temp_cols),
     &(self->temp_pixel_width));

  int actual_height = self->temp_rows * self->row_height;
  int actual_width = self->temp_cols * self->font_width;
//printf ("temp_rows=%d act_height=%d w=%d\n", self->temp_rows, actual_height, actual_width);

  gfx_props.width = actual_width;
  gfx_props.height = actual_height;
  self->gfx_file_desc->devctl 
        (self->gfx_file_desc, DC_GFX_SET_USED_AREA, (int32_t)&gfx_props);      
  self->glyph_buffer = NULL;
  self->row = 0;
  self->col = 0;
  gfxcondev_alloc_buffers (self); 
  }

/*============================================================================
 * gfxcondev_new
* ==========================================================================*/
GfxConDev *gfxcondev_new (const char *name, DevDescriptor *gfxdevdesc)
  {
  GfxConDev *self = malloc (sizeof (GfxConDev));
  memset (self, 0, sizeof (GfxConDev));
  DevDescriptor *desc = malloc (sizeof (DevDescriptor));
  memset (desc, 0, sizeof (DevDescriptor));
  self->desc = desc;
  self->gfxdevdesc = gfxdevdesc;
  desc->name = name;
  desc->get_file_desc = gfxcon_get_file_desc;
  desc->self = self;
  self->flags = DC_TERM_FLAG_ECHO | DC_TERM_FLAG_WRAP | DC_TERM_FLAG_LFISCRLF
                  | DC_TERM_FLAG_BS_NONDESTRUCTIVE;
  self->tab_size = 8;
  self->mode = 0;
  self->csilen = 0;
  return self;
  }

/*============================================================================
 * gfxcondev_destroy
 * ==========================================================================*/
void gfxcondev_destroy (GfxConDev *self)
  {
  self->gfx_file_desc->close (self->gfx_file_desc);
  if (self->glyph_buffer) free (self->glyph_buffer);
  free (self->desc);
  free (self);
  }

/*============================================================================
 * gfxcondev_get_desc
 * ==========================================================================*/
DevDescriptor *gfxcondev_get_desc (GfxConDev *self)
  {
  return self->desc;
  }

