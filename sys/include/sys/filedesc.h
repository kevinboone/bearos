/*============================================================================
 *  sys/filedesc.h
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

#pragma once

#include <stdint.h>
#include <sys/error.h>
#include <sys/limits.h>
#include <time.h>

struct _FileDesc;
typedef Error (*FileDescOpenFn) (struct _FileDesc *f, int flags);
typedef Error (*FileDescCloseFn) (struct _FileDesc *f);
typedef int (*FileDescReadFn) (struct _FileDesc *s, void *buffer, int len);
typedef int (*FileDescReadTimeoutFn) 
                                (struct _FileDesc *s, int msec);
typedef int (*FileDescWriteFn) (struct _FileDesc *s, 
                                 const void *buffer, int len);
typedef int32_t (*FileDescGetSizeFn) (struct _FileDesc *s);
typedef int32_t (*FileDescLSeekFn) (struct _FileDesc *s, int32_t offset, 
                                 int origin);
typedef time_t (*FileDescMTimeFn) (struct _FileDesc *s);
typedef Error (*FileDescDevCtlFn) (struct _FileDesc *s, intptr_t arg1, 
                   intptr_t arg2);
typedef Error (*FileDescTruncateFn) (struct _FileDesc *s, int32_t len);

typedef struct _FileDesc
  {
  char *name;
  void *self;
  FileDescOpenFn open;
  FileDescCloseFn close;
  FileDescReadFn read;
  FileDescReadTimeoutFn read_timeout;
  FileDescWriteFn write;
  FileDescGetSizeFn get_size;
  FileDescLSeekFn lseek;
  FileDescMTimeFn get_mtime;
  FileDescDevCtlFn devctl;
  FileDescTruncateFn truncate;
  int type;
  char reserved[16];
  } FileDesc;

#ifdef __cplusplus
extern "C" {
#endif

extern FileDesc *filedesc_new (void);
extern void filedesc_destroy (FileDesc *self);

#ifdef __cplusplus
}
#endif



