/*============================================================================
 *  sys/sys_devctl.c
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
 * sys_devctl 
 * ==========================================================================*/
int sys_devctl (int fd, intptr_t arg1, intptr_t arg2)
  {
#ifdef TRACE
  TRACE_IN;
#endif

  Process *p = process_get_current();

#ifdef DEBUG
    DEBUG ("fd=%d", fd);
#endif
  if (fd < 0 || fd >= NFILES)
    {
    WARN ("Invalid fd: %d", fd);
    return EINVAL;
    }

  FileDesc *filedesc = 0;
  process_get_filedesc (p, fd, &filedesc);

  if (!filedesc)
    {
    WARN ("Calling devctl on fd that is not open: %d", fd);
    return EINVAL;
    }

  int ret = ENOSYS;

  if (filedesc->devctl)
    {
    ret = filedesc->devctl (filedesc, arg1, arg2);
    }

#ifdef TRACE
  TRACE_OUT;
#endif

  return ret;
  }


intptr_t _sys_devctl (intptr_t fd, intptr_t arg1, intptr_t arg2)
  { 
  return sys_devctl ((int)fd, arg1, arg2);
  }


