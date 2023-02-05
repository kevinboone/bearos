/*============================================================================
 *  fsmanager/fatdir.h
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

#pragma once

/*============================================================================
 * ==========================================================================*/

#include <sys/error.h>
#include <ff.h>

struct _FATDir;
typedef struct _FATDir FATDir;

#ifdef __cplusplus
extern "C" {
#endif

extern FATDir *fatdir_new (const char *path);
extern void fatdir_destroy (FATDir *self);
extern FileDesc *fatdir_get_filedesc (FATDir *self); 

#ifdef __cplusplus
}
#endif







