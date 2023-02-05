/*============================================================================
 *  fsmanager/devfsdir.h
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

#pragma once

/*============================================================================
 * ==========================================================================*/

#include <sys/error.h>
#include <devfs/devfs.h>

struct _DevFSDir;
typedef struct _DevFSDir DevFSDir;

#ifdef __cplusplus
extern "C" {
#endif

extern DevFSDir *devfsdir_new (const DevFS *devfs, const char *path);
extern void devfsdir_destroy (DevFSDir *self);
extern FileDesc *devfsdir_get_filedesc (DevFSDir *self); 

#ifdef __cplusplus
}
#endif






