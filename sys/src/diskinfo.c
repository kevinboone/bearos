/*============================================================================
 *  sys/diskinfo.c
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/error.h>
#include <errno.h>
#include <sys/diskinfo.h>
#include <syslog/syslog.h>
#include <sys/fsmanager.h>

#define TRACE_IN SYSLOG_TRACE_IN
#define TRACE_OUT SYSLOG_TRACE_OUT
#define TRACE SYSLOG_TRACE
#define DEBUG SYSLOG_DEBUG
#define INFO SYSLOG_INFO
#define WARN SYSLOG_WARN

/*============================================================================
 * sys_get_diskinfo
 * ==========================================================================*/
Error sys_get_diskinfo (int8_t drive, DiskInfo *diskinfo)
  {
  TRACE_IN;
  Error ret = 0;

  DEBUG ("Drive=%d", drive);
  FSysDescriptor *fsys = fsmanager_get_descriptor (drive); 
  if (fsys)
    {
    int32_t total_sectors = 0;
    int32_t free_sectors = 0;
    ret = fsys->get_capacity (fsys, &total_sectors, &free_sectors);
    if (ret == 0)
      {
      strncpy (diskinfo->name, fsys->name, SYS_DISKINFO_MAX_NAME); 
      diskinfo->name[SYS_DISKINFO_MAX_NAME - 1] = 0;
      diskinfo->total_sectors = total_sectors;
      diskinfo->free_sectors = free_sectors;
      }
    }
  else 
    ret = EINVAL;

  TRACE_OUT;
  return ret;
  }


