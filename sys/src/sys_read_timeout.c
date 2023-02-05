/*============================================================================
 *  sys/sys_read_timeout.c
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
 * sys_read_timeout 
 * ==========================================================================*/
int sys_read_timeout (int fd, int msec)
  {
  TRACE_IN;
#ifdef DEBUG
    DEBUG ("fd=%d", fd);
#endif
  Process *p = process_get_current();

  if (fd < 0 || fd >= NFILES)
    {
    WARN ("Invalid fd: %d", fd);
    return EINVAL;
    }

  FileDesc *filedesc = 0;
  process_get_filedesc (p, fd, &filedesc);

  if (!filedesc)
    {
    WARN ("Reading fd that is not open: %d", fd);
    return EINVAL;
    }
  
  int c = filedesc->read_timeout (filedesc, msec);

  TRACE_OUT;
  return c;
  }

intptr_t _sys_read_timeout (intptr_t fd, intptr_t msec, intptr_t notused)
  {
  (void)notused;
  return sys_read_timeout ((int)fd, (int)msec);
  }




