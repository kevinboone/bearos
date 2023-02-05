/*============================================================================
 *  fsmanager/fsmanager.h
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
#include <sys/fsysdesc.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void fsmanager_init (void);
extern Error fsmanager_mount (int8_t drive, FSysDescriptor *descriptor);
extern Error fsmanager_unmount (int8_t drive);
extern FSysDescriptor *fsmanager_get_descriptor (int8_t drive);
extern FSysDescriptor *fsmanager_get_descriptor_by_path (const char *path);


#ifdef __cplusplus
}
#endif





