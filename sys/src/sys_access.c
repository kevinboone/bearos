/*============================================================================
 *  sys/sys_access.c
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
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
 * sys_access
 * ==========================================================================*/
int sys_access (const char *path, int mode)
  {
  TRACE_IN;
#ifdef DEBUG
  DEBUG ("path=%s", path);
#endif
  (void)mode;

  int ret = ENOENT;

  // TODO BearOS does not support file permissions at all at present, since
  //   the only supported filesystem is FAT. So, whatever the setting of
  //   'mode', if the file exists at all, that's sufficient. The only
  //   way in which this function will return non-zero is if the file
  //   does not actually exist.
  // If we ever support more sophisticated filesystems, we'll have to
  //   extend this functionality considerably.

  int fd = sys_open (path, O_RDONLY);
  if (fd >= 0)
    {
    ret = 0;
    sys_close (fd);
    }

  return ret;
  }

intptr_t _sys_access (intptr_t path, intptr_t mode, intptr_t unused)
  {
  (void)unused;
  return sys_access ((const char *)path, (int)mode);
  }





