/*============================================================================
 *  sys/sys_ftruncate.c
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
 * sys_ftruncate
 * ==========================================================================*/
int32_t sys_ftruncate (int fd, int32_t len)
  {
  TRACE_IN;
#ifdef DEBUG
    DEBUG ("fd=%d len=%d", fd, len);
#endif
  Process *p = process_get_current();

  if (fd < 0 || fd >= NFILES)
    {
    WARN ("Invalid fd: %d", fd);
    return -EINVAL;
    }

  FileDesc *filedesc = 0;
  process_get_filedesc (p, fd, &filedesc);

  if (!filedesc)
    {
    WARN ("Truncate fd that is not open: %d", fd);
    return -EINVAL;
    }
  
  int ret;

  if (filedesc->truncate)
    ret = filedesc->truncate (filedesc, len);
  else
    ret = -ENOSYS;

  TRACE_OUT;
  return ret;
  }

intptr_t _sys_ftruncate (intptr_t fd, intptr_t len, intptr_t notused)
  {
  (void)notused;
  return sys_ftruncate ((int)fd, (int32_t)len);
  }




