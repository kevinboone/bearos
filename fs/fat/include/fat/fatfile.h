/*============================================================================
 *  fsmanager/fatfile.h
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

#pragma once

/*============================================================================
 * ==========================================================================*/

#include <sys/error.h>
#include <ff.h>

struct _FATFile;
typedef struct _FATFile FATFile;

#ifdef __cplusplus
extern "C" {
#endif

extern FATFile *fatfile_new (const char *path, int flags);
extern void fatfile_destroy (FATFile *self);
extern FileDesc *fatfile_get_filedesc (FATFile *self); 

#ifdef __cplusplus
}
#endif








