/*============================================================================
 *  chardev/i2clcddev.c
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

/*============================================================================
 * ==========================================================================*/

#include <stdint.h>
#include <stdlib.h>
#include <pico/stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/error.h>
#include <errno.h>
#include <fatfs_loopback/fatloopback.h>
#include <term/term.h>
#include <chardev/i2clcddev.h>
#include <i2c_lcd/i2c_lcd.h>

struct _I2CLCDDev
  {
  I2C_LCD *i2c_lcd;
  DevDescriptor *dev_desc;
  char *name;
  };

/*============================================================================
 * i2clcddev_close
 * ==========================================================================*/
Error i2clcddev_close (FileDesc *descriptor)
  {
  if (descriptor->name) free (descriptor->name);
  free (descriptor);
  //I2CLCDDev *self = descriptor->self;
  //i2clcddev_destroy (self);
  return 0;
  }

/*============================================================================
 * i2clcddev_write
 * ==========================================================================*/
int i2clcddev_write (FileDesc *descriptor, const void* buffer, int c)
  {
  (void)descriptor; (void)buffer; (void)c;
  I2CLCDDev *self = descriptor->self;
  return i2c_lcd_write (self->i2c_lcd, buffer, c);
  }

/*============================================================================
 * i2clcddev_read
 * ==========================================================================*/
int i2clcddev_read (FileDesc *descriptor, void *buffer, int len)
  {
  (void) descriptor; (void)buffer; (void)len;
  return -ENOSYS;
  }

/*============================================================================
 * i2clcddev_read
 * ==========================================================================*/
int i2clcddev_read_timeout (FileDesc *descriptor, int msec)
  {
  (void)descriptor; (void)msec;
  return -ENOSYS;
  }

/*============================================================================
 * i2clcddev_init
 * ==========================================================================*/
#if PICO_ON_DEVICE
Error i2clcddev_init (I2CLCDDev *self, int width, int height, int addr, 
       int i2c_num, int sda, int scl, int i2c_baud, int scrollback_pages)
  {
  self->i2c_lcd = i2c_lcd_new (width, height, addr, i2c_num == 0 ? i2c0 : i2c1,
           sda, scl, i2c_baud, scrollback_pages);

  // Write some initial text, so we know the display is working
  i2c_lcd_set_cursor (self->i2c_lcd, 0, 0);
  i2c_lcd_print_string (self->i2c_lcd, "BearOS ");
  return 0;
  }
#else
Error i2clcddev_init (I2CLCDDev *self, int width, int height)
  {
  self->i2c_lcd = i2c_lcd_new (width, height);
  // Write some initial text, so we know the display is working
  i2c_lcd_set_cursor (self->i2c_lcd, 0, 0);
  i2c_lcd_print_string (self->i2c_lcd, "BearOS ");
  return 0;
  }
#endif

/*============================================================================
 * i2clcddevde_get_file_desc
* ==========================================================================*/
FileDesc *i2clcddev_get_file_desc (DevDescriptor *dev_desc)
  {
  I2CLCDDev *self = dev_desc->self;
  FileDesc *f = malloc (sizeof (FileDesc));
  memset (f, 0, sizeof (FileDesc));
  f->self = self;
  f->name = strdup (self->name);
  f->close = i2clcddev_close;
  f->write = i2clcddev_write;
  f->read = i2clcddev_read;
  return f;
  }

/*============================================================================
 * i2clcddev_new
 * ==========================================================================*/
I2CLCDDev *i2clcddev_new (const char *name)
  {
  I2CLCDDev *self = malloc (sizeof (I2CLCDDev));
  memset (self, 0, sizeof (I2CLCDDev));
  DevDescriptor *dev_desc = malloc (sizeof (DevDescriptor));
  memset (dev_desc, 0, sizeof (DevDescriptor));
  self->name = strdup (name);
  dev_desc->name = self->name; 
  dev_desc->get_file_desc = i2clcddev_get_file_desc;
  dev_desc->self = self;
  self->dev_desc = dev_desc;
  return self;
  }

/*============================================================================
 * i2clcddev_destroy
 * ==========================================================================*/
void i2clcddev_destroy (I2CLCDDev *self)
  {
  if (self->i2c_lcd) i2c_lcd_destroy (self->i2c_lcd);
  free (self->dev_desc);
  free (self->name);
  free (self);
  }

/*============================================================================
 * i2clcddev_get_desc
 * ==========================================================================*/
DevDescriptor *i2clcddev_get_desc (I2CLCDDev *self)
  {
  return self->dev_desc;
  }


