/*============================================================================
 *  chardev/i2clcddev.h
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

#pragma once

/*============================================================================
 * ==========================================================================*/

#include <stdint.h>
#include <sys/error.h>
#include <sys/filedesc.h>
#include <devmgr/devmgr.h>
#if PICO_ON_DEVICE
#include <hardware/i2c.h>
#endif

struct _I2CLCDDev;
typedef struct _I2CLCDDev I2CLCDDev;

#ifdef __cplusplus
extern "C" {
#endif

extern I2CLCDDev *i2clcddev_new (const char *name);
extern void i2clcddev_destroy (I2CLCDDev *self);
extern DevDescriptor *i2clcddev_get_desc (I2CLCDDev *self); 

#if PICO_ON_DEVICE
extern Error i2clcddev_init (I2CLCDDev *self, int width, int height, int addr, 
       int i2c_num, int sda, int scl, int i2c_baud, int scrollback_pages);
#else
extern Error i2clcddev_init (I2CLCDDev *self, int width, int height);
#endif

#ifdef __cplusplus
}
#endif






