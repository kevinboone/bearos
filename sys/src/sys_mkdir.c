/*============================================================================
 *  sys/sys_mkdir.c
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
#include <sys/fsutil.h>
#include <sys/fsysdesc.h>
#include <sys/fsmanager.h>
#include <sys/syscalls.h>
#include <sys/filedesc.h>
#include <sys/process.h>

#define TRACE_IN SYSLOG_TRACE_IN
#define TRACE_OUT SYSLOG_TRACE_OUT
#define TRACE SYSLOG_TRACE
#define DEBUG SYSLOG_DEBUG
#define INFO SYSLOG_INFO
#define WARN SYSLOG_WARN

/*============================================================================
 * sys_mkdir 
 * ==========================================================================*/
int sys_mkdir (const char *path)
  {
  TRACE_IN;
#ifdef DEBUG
  DEBUG ("path=%s", path);
#endif
  char abspath[PATH_MAX];
  fsutil_make_abs_path (path, abspath, PATH_MAX);

  FSysDescriptor *desc = fsmanager_get_descriptor_by_path (abspath);
  if (desc)
    {
    if (desc->mkdir)
      return desc->mkdir (desc, abspath + 2);
    else
      return ENOSYS;
    }
  else
    return ENOENT;
  }

intptr_t _sys_mkdir (intptr_t path, intptr_t notused1, intptr_t notused2)
  {
  (void)notused1; (void)notused2;
  return sys_mkdir ((const char *)path);
  }



