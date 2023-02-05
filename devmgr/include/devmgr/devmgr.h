/*============================================================================
 *  devmgr/devmgr.h
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

#pragma once

/*============================================================================
 * ==========================================================================*/

#include <stdint.h>
#include <sys/error.h>
#include <sys/filedesc.h>

#define DEVMGR_MAX_DEVS 16

struct _DevDescriptor;

typedef FileDesc *(*DMGetFileDescFn)(struct _DevDescriptor *desc);

typedef struct _DevDescriptor 
  {
  const char *name;
  void *self;
  DMGetFileDescFn get_file_desc;
  } DevDescriptor;

#ifdef __cplusplus
extern "C" {
#endif

extern void devmgr_init (void);
extern void devmgr_deinit (void);
extern void devmgr_register (DevDescriptor *desc);
extern DevDescriptor *devmgr_find_descriptor (const char *name);
extern int devmgr_get_num_devs (void);
extern DevDescriptor *devmgr_get_desc (int n);

#ifdef __cplusplus
}
#endif


