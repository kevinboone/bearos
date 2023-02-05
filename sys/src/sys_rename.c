/*============================================================================
 *  sys/sys_rename.c
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

/*============================================================================
 * ==========================================================================*/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pico/stdlib.h>
#include <stdio.h>
#include <sys/process.h>
#include <sys/error.h>
#include <errno.h>
#include <syslog/syslog.h>
#include <sys/syscalls.h>
#include <sys/fsmanager.h>
#include <sys/fsutil.h>

#define TRACE SYSLOG_TRACE
#define DEBUG SYSLOG_DEBUG
#define INFO SYSLOG_INFO
#define WARN SYSLOG_WARN

/*=========================================================================
  sys_getrename
=========================================================================*/
int sys_rename (const char *source, const char *target)
  {
#ifdef DEBUG
  DEBUG ("source=%s, target=%s", source, target);
#endif
  char real_source[PATH_MAX];
  char real_target[PATH_MAX];
  Error ret = 0;
  fsutil_make_abs_path (source, real_source, PATH_MAX);
  fsutil_make_abs_path (target, real_target, PATH_MAX);
  if (real_source[0] == real_target[0])
     {
     FSysDescriptor *desc = fsmanager_get_descriptor_by_path (real_source);
     if (desc)
       {
       if (desc->rename)
         {
         ret = desc->rename (desc, real_source + 2, real_target + 2);
         }
       else
         ret = ENOSYS;
       }
    else
      ret = ENOENT;
    }
  else
    ret = EIO;

#ifdef DEBUG
  DEBUG ("Done, ret=%d", ret);
#endif
  return ret;
  }

intptr_t _sys_rename (intptr_t from_path, intptr_t to_path, intptr_t unused)
  {
  (void)unused;
  return sys_rename ((const char *)from_path, (const char *)to_path);
  }




