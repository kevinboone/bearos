/*============================================================================
 *  chardev/wslcddev.c
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
#include <bearos/gpio.h>
#include <string.h>
#include <sys/error.h>
#include <errno.h>
#include <chardev/wslcddev.h>
#include <waveshare_lcd/waveshare_lcd.h>

struct _WSLCDDev
  {
  char *name;
  DevDescriptor *desc;
  WSLCD *wslcd;
  // expected_bytes is the width * height of the most recent window-setting
  //   operation. Once the window is set, this much data _must_ be sent.
  int expected_bytes;
  };

/*============================================================================
 * wslcddev_read
* ==========================================================================*/
int wslcddev_read (FileDesc *f, void *buffer, int len)
  {
  return 0;
  }

/*============================================================================
 * wslcddev_write
* ==========================================================================*/
int wslcddev_write (FileDesc *f, const void *buffer, int len)
  {
  WSLCDDev *self = f->self;
  //printf ("transfer %d bytes\n", len);
  wslcd_transfer (self->wslcd, buffer, len / 2); 
  return len;
  }

/*============================================================================
 * wslcddev_close
* ==========================================================================*/
Error wslcddev_close (FileDesc *f)
  {
  free (f);
  return 0;
  }

/*============================================================================
 * wslcddev_devctl
* ==========================================================================*/
Error wslcddev_devctl (FileDesc *f, intptr_t arg1, intptr_t arg2)
  {
  WSLCDDev *self = f->self;
  switch (arg1)
    {
    case DC_GFX_RESET_AND_CLEAR:
      uint16_t black = 0;
      wslcd_clear (self->wslcd, black);
      break;

    case DC_GFX_GET_PROPS:
      DevCtlGfxProps *props = (DevCtlGfxProps*)arg2;
      props->width = wslcd_get_width (self->wslcd);
      props->height = wslcd_get_height (self->wslcd);
      props->colmode = DC_GFX_COLMODE_RGB565;
      return 0;

    case DC_GFX_SET_USED_AREA:
      props = (DevCtlGfxProps*)arg2;
      wslcd_set_used_area (self->wslcd, props->width, props->height);
      return 0;

    case DC_GFX_SET_REGION:
      DevCtlGfxRegion *region = (DevCtlGfxRegion *)arg2;
#if PICO_ON_DEVICE
      wslcd_set_window (self->wslcd, (uint16_t) region->x, 
        (uint16_t) region->y, 
        (uint16_t)(region->x + region->cx), 
        (uint16_t) (region->y + region->cy));
#endif
      self->expected_bytes = region->cx * region->cy;
      return 0;

    case DC_GFX_FILL:
#if PICO_ON_DEVICE
      DevCtlGfxColour *colour = (DevCtlGfxColour *)arg2;
      uint16_t rgb = colour->rgb565; // This device only supports RGB565
      wslcd_send_repeated_word (self->wslcd, rgb, self->expected_bytes);
#endif
      self->expected_bytes = 0; 
      return 0;

    case DC_GFX_SCROLL_UP:
#if PICO_ON_DEVICE
      wslcd_scroll_up (self->wslcd, (uint16_t)arg2);
#endif
      return 0;

    case DC_GFX_WRITE:
      DevCtlGfxWrite *wr = (DevCtlGfxWrite *)arg2;
      Error ret = wslcddev_write (f, wr->buffer, wr->length);
      self->expected_bytes = 0; 
      return ret;

    case DC_GET_GEN_FLAGS:
      *((int32_t *)arg2) = DC_FLAG_ISGFX;
    }
  return EINVAL;
  }

/*============================================================================
 * wslcddev_get_file_desc
* ==========================================================================*/
FileDesc *wslcddev_get_file_desc (DevDescriptor *dev_desc)
  {
  WSLCDDev *self = dev_desc->self;
  FileDesc *f = malloc (sizeof (FileDesc));
  memset (f, 0, sizeof (FileDesc));
  f->self = self;
  f->close = wslcddev_close;
  f->write = wslcddev_write;
  f->read = wslcddev_read;
  f->read_timeout = NULL;
  f->devctl = wslcddev_devctl;
  return f;
  }

/*============================================================================
 * wslcddev_new
* ==========================================================================*/
WSLCDDev *wslcddev_new (const char *name)
  {
  WSLCDDev *self = malloc (sizeof (WSLCDDev));
  DevDescriptor *desc = malloc (sizeof (DevDescriptor));
  memset (desc, 0, sizeof (DevDescriptor));
  self->desc = desc;
  desc->name = name;
  desc->get_file_desc = wslcddev_get_file_desc;
  desc->self = self;
  self->expected_bytes = 0;
  return self;
  }

/*============================================================================
 * wslcddev_destroy
 * ==========================================================================*/
void wslcddev_destroy (WSLCDDev *self)
  {
  if (self->wslcd) wslcd_destroy (self->wslcd);
  free (self->desc);
  free (self);
  }

/*============================================================================
 * wslcddev_get_desc
 * ==========================================================================*/
DevDescriptor *wslcddev_get_desc (WSLCDDev *self)
  {
  return self->desc;
  }

/*============================================================================
 * wslcddev_init
 * ==========================================================================*/
#if PICO_ON_DEVICE
void wslcddev_init (WSLCDDev *self, spi_inst_t *spi, uint gpio_cs, 
    uint gpio_miso, uint gpio_mosi, uint gpio_sck, uint gpio_rst, uint gpio_dc, 
    uint gpio_bl, int baud_rate, WSLCDScanDir scan_dir)
  {
  self->wslcd = wslcd_new (spi, gpio_cs, gpio_miso, gpio_mosi, gpio_sck,
     gpio_rst, gpio_dc, gpio_bl, baud_rate, scan_dir);
  wslcd_init (self->wslcd);
  }
#else
void wslcddev_init (WSLCDDev *self)
  {
  self->wslcd = wslcd_new ();
  wslcd_init (self->wslcd);
  }  

#endif


