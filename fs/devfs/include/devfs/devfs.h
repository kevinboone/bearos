/*============================================================================
 *  fsmanager/devfs.h
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

#pragma once

/*============================================================================
 * ==========================================================================*/

#include <sys/error.h>
#include <sys/fsysdesc.h>

struct _DevFS;
typedef struct _DevFS DevFS;

#ifdef __cplusplus
extern "C" {
#endif

extern DevFS *devfs_new (void);
extern void devfs_destroy (DevFS *self);
extern FSysDescriptor *devfs_get_descriptor (const DevFS *self);
extern int devfs_get_device_count (const DevFS *self);
extern const char *devfs_get_device_name (const DevFS *self, int n);


#ifdef __cplusplus
}
#endif






