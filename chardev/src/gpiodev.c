/*============================================================================
 *  chardev/gpiodev.c
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
#include <fatfs_loopback/fatloopback.h>
#include <chardev/gpiodev.h>

struct _GPIODev
  {
  char *name;
  DevDescriptor *desc;
  int last_read;
  };

/*============================================================================
 * gpiodev_read
* ==========================================================================*/
int gpiodev_read (FileDesc *f, void *buffer, int len)
  {
  if (len > 2) return -EINVAL;
  GPIODev *self = f->self;
  ((char *)buffer)[0] = (char)(self->last_read + '0');
  ((char *)buffer)[1] = 0; 
  return 2;
  }

/*============================================================================
 * gpiodev_write
* ==========================================================================*/
int gpiodev_write (FileDesc *f, const void *buffer, int len)
  {
  GPIODev *self = (GPIODev *)f;
  // FORMAT
  // ccpp 
  // cc is a two-letter command, pp is a two-digit pin
  if (len < 4)
    return -EINVAL;
   
  char s_pin[3];
  s_pin[0] = ((char *)buffer)[2]; 
  s_pin[1] = ((char *)buffer)[3]; 
  s_pin[2] = 0;
  int pin = atoi (s_pin); // We're going for speed here -- no error checks :/ 

  if (strncmp ((char *)buffer, GPIOCMD_INIT, 2) == 0)
    {
#if PICO_ON_DEVICE
    gpio_init ((uint)pin);
#else
    printf ("gpio_init pin %d\n", pin);
#endif
    } 
  else if (strncmp ((char *)buffer, GPIOCMD_READ, 2) == 0)
    {
#if PICO_ON_DEVICE
    self->last_read = gpio_get ((uint)pin);
#else
    printf ("gpio_get pin %d\n", pin);
    self->last_read = 0;
#endif
    } 
  else if (strncmp ((char *)buffer, GPIOCMD_PULL_UP, 2) == 0)
    {
#if PICO_ON_DEVICE
    gpio_pull_up ((uint)pin);
#else
    printf ("gpio_pull_up pin %d\n", pin);
#endif
    } 
  else if (strncmp ((char *)buffer, GPIOCMD_PULL_DOWN, 2) == 0)
    {
#if PICO_ON_DEVICE
    gpio_pull_down ((uint)pin);
#else
    printf ("gpio_pull_down pin %d\n", pin);
#endif
    } 
  else if (strncmp ((char *)buffer, GPIOCMD_SET_HIGH, 2) == 0)
    {
#if PICO_ON_DEVICE
    gpio_put ((uint)pin, 1);
#else
    printf ("gpio_set pin %d = 1\n", pin);
#endif
    } 
  else if (strncmp ((char *)buffer, GPIOCMD_SET_LOW, 2) == 0)
    {
#if PICO_ON_DEVICE
    gpio_put ((uint)pin, 0);
#else
    printf ("gpio_set pin %d = 0\n", pin);
#endif
    } 
  else if (strncmp ((char *)buffer, GPIOCMD_SET_INPUT, 2) == 0)
    {
#if PICO_ON_DEVICE
    gpio_set_dir ((uint)pin, 0);
#else
    printf ("gpio_set_dir pin %d = 0\n", pin);
#endif
    } 
  else if (strncmp ((char *)buffer, GPIOCMD_SET_OUTPUT, 2) == 0)
    {
#if PICO_ON_DEVICE
    gpio_set_dir ((uint)pin, 1);
#else
    printf ("gpio_set_dir pin %d = 1\n", pin);
#endif
    } 
  else 
    return -EINVAL;

  return len;
  }

/*============================================================================
 * gpiodev_close
* ==========================================================================*/
Error gpiodev_close (FileDesc *f)
  {
  free (f);
  return 0;
  }

/*============================================================================
 * gpiodev_get_file_desc
* ==========================================================================*/
FileDesc *gpiodev_get_file_desc (DevDescriptor *dev_desc)
  {
  GPIODev *self = dev_desc->self;
  FileDesc *f = malloc (sizeof (FileDesc));
  memset (f, 0, sizeof (FileDesc));
  f->self = self;
  f->close = gpiodev_close;
  f->write = gpiodev_write;
  f->read = gpiodev_read;
  f->read_timeout = NULL;
  f->devctl = NULL;
  return f;
  }

/*============================================================================
 * gpiodev_new
* ==========================================================================*/
GPIODev *gpiodev_new (const char *name)
  {
  GPIODev *self = malloc (sizeof (GPIODev));
  DevDescriptor *desc = malloc (sizeof (DevDescriptor));
  memset (desc, 0, sizeof (DevDescriptor));
  self->last_read = 0;
  self->desc = desc;
  desc->name = name;
  desc->get_file_desc = gpiodev_get_file_desc;
  desc->self = self;
  return self;
  }

/*============================================================================
 * gpiodev_destroy
 * ==========================================================================*/
void gpiodev_destroy (GPIODev *self)
  {
  free (self->desc);
  free (self);
  }

/*============================================================================
 * gpiodev_get_desc
 * ==========================================================================*/
DevDescriptor *gpiodev_get_desc (GPIODev *self)
  {
  return self->desc;
  }



