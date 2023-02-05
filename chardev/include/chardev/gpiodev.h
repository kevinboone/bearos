/*============================================================================
 *  chardev/gpiodev.h
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

#pragma once

/*============================================================================
 * ==========================================================================*/

#include <stdint.h>
#include <sys/error.h>
#include <devmgr/devmgr.h>

struct _GPIODev;
typedef struct _GPIODev GPIODev;

#ifdef __cplusplus
extern "C" {
#endif

extern GPIODev *gpiodev_new (const char *name);
extern void gpiodev_destroy (GPIODev *self);
extern DevDescriptor *gpiodev_get_desc (GPIODev *self); 

#ifdef __cplusplus
}
#endif







