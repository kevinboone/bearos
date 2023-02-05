/*============================================================================
 *  sys/diskinfo.h
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

#pragma once

#include <stdint.h>
#include <sys/error.h>

#define SYS_DISKINFO_MAX_NAME 32

typedef struct _DiskInfo
  {
  char name[SYS_DISKINFO_MAX_NAME];
  int32_t total_sectors;
  int32_t free_sectors;
  } DiskInfo;

#ifdef __cplusplus
extern "C" {
#endif

Error sys_get_diskinfo (int8_t, DiskInfo *diskinfo);


#ifdef __cplusplus
}
#endif



