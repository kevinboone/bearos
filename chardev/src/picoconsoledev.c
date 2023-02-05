/*============================================================================
 *  chardev/picoconsoledev.c
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
#if PICO_ON_DEVICE
#else
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
static struct termios orig_termios;
#endif

#include <string.h>
#include <sys/error.h>
#include <errno.h>
#include <fatfs_loopback/fatloopback.h>
#include <chardev/picoconsoledev.h>

// Nasty end-of-input flag, which is global because I've been tool lazy
//   to define a specific object for each file which is based on this
//   driver. Still, there is only one console, so we might get away 
//   with it.
static bool eoi = false;

struct _PicoConsoleDev
  {
  char *name;
  DevDescriptor *desc;
  int32_t flags;
  };

/*============================================================================
 * picoconsolede_read
* ==========================================================================*/
int picoconsole_read (FileDesc *f, void *buffer, int len)
  {
  PicoConsoleDev *self = f->self;
  if (eoi) return 0;
  for (int i = 0; i < len; i++)
    {
    int c;
#if PICO_ON_DEVICE
    while ((c = getchar_timeout_us (0)) < 0)
      {
      // gpio_put (LED_PIN, 1);
      // sleep_ms (50);
      // gpio_put (LED_PIN, 0);
      sleep_ms (1);
      }
    if (c == I_EOI) { eoi = true; return i; }
    ((char *)buffer)[i] = (char)c;
    if (c == I_EOL) { return i + 1; } // TODO
#else
    while ((c = getchar()) < 0)
      {
      if (c == 0) return 0;
      sleep_ms (10);
      }
    if (c == I_EOI) { eoi = true; return i; }
    ((char *)buffer)[i] = (char)c;
#endif
    if (!(self->flags & DC_TERM_FLAG_NOECHO)) putchar (c);
    }
  return len;
  }

/*============================================================================
 * picoconsolede_read_timeout
* ==========================================================================*/
int picoconsole_read_timeout (FileDesc *f, int msec)
  {
  PicoConsoleDev *self = f->self;
  
  int c;
#if PICO_ON_DEVICE
  int loops = 0;
  while ((c = getchar_timeout_us (0)) < 0 && loops < msec)
    {
    sleep_us (1000);
    loops++;
    }
  return c;
#else
  (void)msec; (void)self;
  int loops = 0;
  while ((c = getchar ()) < 0 && loops < msec)
    {
    usleep (1000);
    loops++;
    }
#endif
  // NOT SURE ABOUT THIS...
  //if (!(self->flags & DC_TERM_FLAG_NOECHO)) putchar (c);
  return c;
  }

/*============================================================================
 * picoconsolede_write
* ==========================================================================*/
int picoconsole_write (FileDesc *f, const void *buffer, int len)
  {
  (void)f;
  for (int i = 0; i < len; i++)
    putchar (((char *)buffer)[i]);
  return len;
  }

/*============================================================================
 * picoconsolede_close
* ==========================================================================*/
Error picoconsole_close (FileDesc *f)
  {
  eoi = false;
  free (f);
  return 0;
  }

/*============================================================================
 * picoconsolede_ioctl
* ==========================================================================*/
Error picoconsole_devctl (FileDesc *f, intptr_t arg1, intptr_t arg2)
  {
  PicoConsoleDev *self = f->self;

  switch (arg1)
    {
    case DC_TERM_SET_FLAGS:
      self->flags = (int32_t)arg2;
      return 0;
    case DC_TERM_GET_FLAGS:
      *((int32_t *)arg2) = self->flags;
      return 0;
    case DC_TERM_GET_PROPS:
      DevCtlTermProps *props = (DevCtlTermProps*)arg2;
      props->rows = 23;
      props->cols = 80;
      return 0;
    case DC_GET_GEN_FLAGS:
      *((int32_t *)arg2) = DC_FLAG_ISTTY;
      return 0;
    } 
  return EINVAL;
  }

/*============================================================================
 * picoconsolede_get_file_desc
* ==========================================================================*/
FileDesc *picoconsole_get_file_desc (DevDescriptor *dev_desc)
  {
  PicoConsoleDev *self = dev_desc->self;
  FileDesc *f = malloc (sizeof (FileDesc));
  memset (f, 0, sizeof (FileDesc));
  eoi = false;
  f->self = self;
  f->close = picoconsole_close;
  f->write = picoconsole_write;
  f->read = picoconsole_read;
  f->read_timeout = picoconsole_read_timeout;
  f->devctl = picoconsole_devctl;
  return f;
  }

/*============================================================================
 * picoconsolede_new
* ==========================================================================*/
PicoConsoleDev *picoconsoledev_new (const char *name)
  {
  PicoConsoleDev *self = malloc (sizeof (PicoConsoleDev));
  DevDescriptor *desc = malloc (sizeof (DevDescriptor));
  memset (desc, 0, sizeof (DevDescriptor));
  self->desc = desc;
  desc->name = name;
  desc->get_file_desc = picoconsole_get_file_desc;
  desc->self = self;
  self->flags = DC_TERM_FLAG_ECHO;

#if PICO_ON_DEVICE
#else
  tcgetattr (STDIN_FILENO, &orig_termios);
  struct termios raw = orig_termios;
  raw.c_iflag &= (unsigned int) ~(IXON);
  raw.c_lflag &= (unsigned int) ~(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_cc[VTIME] = 1;
  raw.c_cc[VMIN] = 0;
  tcsetattr (STDIN_FILENO, TCSAFLUSH, &raw);
#endif

  return self;
  }

/*============================================================================
 * picoconsoledev_destroy
 * ==========================================================================*/
void picoconsoledev_destroy (PicoConsoleDev *self)
  {
#if PICO_ON_DEVICE
#else
  tcsetattr (STDIN_FILENO, TCSAFLUSH, &orig_termios);
#endif
  free (self->desc);
  free (self);
  }

/*============================================================================
 * picoconsoledev_get_desc
 * ==========================================================================*/
DevDescriptor *picoconsoledev_get_desc (PicoConsoleDev *self)
  {
  return self->desc;
  }


