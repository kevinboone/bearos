/*============================================================================
 *  chardev/picoconsoledev.h
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

#pragma once

/*============================================================================
 * ==========================================================================*/

#include <stdint.h>
#include <sys/error.h>
#include <devmgr/devmgr.h>

struct _PicoConsoleDev;
typedef struct _PicoConsoleDev PicoConsoleDev;

#ifdef __cplusplus
extern "C" {
#endif

extern PicoConsoleDev *picoconsoledev_new (const char *name);
extern void picoconsoledev_destroy (PicoConsoleDev *self);
extern DevDescriptor *picoconsoledev_get_desc (PicoConsoleDev *self); 

#ifdef __cplusplus
}
#endif






