/*============================================================================
 *  sys/direntry.h
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

#pragma once

#include <sys/stat.h>
#include <sys/limits.h>
#include <time.h>

typedef struct _DirEntry
  {
  char name[PATH_MAX];
  int type;
  time_t mtime;
  int32_t size;
  } DirEntry;

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif




