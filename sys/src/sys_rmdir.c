/*============================================================================
 *  sys/sys_rmdir.c
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
#include <sys/fsutil.h>
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
 * sys_rmdir
 * ==========================================================================*/
int sys_rmdir (const char *path)
  {
  TRACE_IN;
#ifdef DEBUG
  DEBUG ("path=%s", path);
#endif
  Error ret;
  char abspath[PATH_MAX];
  fsutil_make_abs_path (path, abspath, PATH_MAX);
  if (fsutil_is_directory (abspath) == false)
    {
    ret = EINVAL;
    // We won't delete a file, even if the underlying filesystem
    //   supports file  deletion.
    }
  else
    {
    FSysDescriptor *desc = fsmanager_get_descriptor_by_path (abspath);
    if (desc)
      {
      if (desc->rmdir)
	ret = desc->rmdir (desc, abspath + 2);
      else
	ret = ENOSYS;
      }
    else
      ret = ENOENT;
    }
#ifdef DEBUG
  DEBUG ("ret=%d", ret);
#endif
  DEBUG ("printf=%d", ret);
  return ret;
  }

intptr_t _sys_rmdir (intptr_t path, intptr_t unused1, intptr_t unused2)
  {
  (void)unused1; (void)unused2;
  return sys_rmdir ((const char *)path);
  }





