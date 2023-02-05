/*============================================================================
 *  sys/fsysdescriptor.h
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

#pragma once

/*============================================================================
 * ==========================================================================*/

#include <stdint.h>
#include <utime.h>
#include <sys/error.h>
#include <sys/filedesc.h>

struct _FSysDescriptor;

typedef Error (*FSMountFn) (struct _FSysDescriptor *self);
typedef Error (*FSUnmountFn) (struct _FSysDescriptor *self);
typedef Error (*FSMkdirFn) (struct _FSysDescriptor *self, const char *path);
typedef Error (*FSGetCapacityFn) (struct _FSysDescriptor *self, 
                 int32_t *total_sectors, int32_t *free_sectors);
typedef FileDesc *(*FSGetFileDescFn) (struct _FSysDescriptor *self, 
                 const char *path, int flags, Error *error);
typedef Error (*FSRenameFn) (struct _FSysDescriptor *self, 
                 const char *source, const char *target);
typedef Error (*FSUnlinkFn) (struct _FSysDescriptor *self, 
                 const char *path);
typedef Error (*FSRmdirFn) (struct _FSysDescriptor *self, 
                 const char *path);
typedef Error (*FSUtimeFn) (struct _FSysDescriptor *self, 
                 const char *path, const struct utimbuf *times);

typedef struct _FSysDescriptor 
  {
  char *name;
  FSMountFn mount;
  FSUnmountFn unmount;
  FSGetCapacityFn get_capacity;
  FSGetFileDescFn get_filedesc;
  FSMkdirFn mkdir;
  FSUnlinkFn unlink;
  FSRmdirFn rmdir;
  FSRenameFn rename;
  FSUtimeFn utime;
  void *self;
  } FSysDescriptor;


