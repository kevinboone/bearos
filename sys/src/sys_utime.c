/*============================================================================
 *  sys/sys_utime.c
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <utime.h>
#include <sys/error.h>
#include <sys/fsmanager.h>
#include <sys/fsutil.h>
#include <errno.h>
#include <syslog/syslog.h>
#include <sys/syscalls.h>

/*============================================================================
 * sys_utime 
 * ==========================================================================*/
int sys_utime (const char *path, const struct utimbuf *times)
  {
  Error ret = 0;
  char abspath[PATH_MAX];
  fsutil_make_abs_path (path, abspath, PATH_MAX);

  FSysDescriptor *desc = fsmanager_get_descriptor_by_path (abspath);
  if (desc)
    {
    if (desc->utime)
      {
      ret = desc->utime (desc, abspath + 2, times);
      }
    else
      ret = ENOSYS;
    }
  else
    ret = ENOENT;
  return ret;
  }

intptr_t _sys_utime (intptr_t path, intptr_t times, intptr_t notused)
  {
  (void)notused;
  return (int32_t) sys_utime ((const char *)path, 
     (const struct utimbuf*) times);
  }


