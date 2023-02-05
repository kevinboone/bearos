/*============================================================================
 *  sys/sys_read.c
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
 * sys_read 
 * ==========================================================================*/
int sys_read (int fd, void *buffer, int len)
  {
  TRACE_IN;
#ifdef DEBUG
    DEBUG ("fd=%d, l=%d", fd, len);
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
    WARN ("Reading fd that is not open: %d", fd);
    return -EINVAL;
    }
  
  Error ret = filedesc->read (filedesc, buffer, len);

  TRACE_OUT;
  return ret;
  }


intptr_t _sys_read (intptr_t fd, intptr_t buffer, intptr_t len)
  {
  return sys_read ((int)fd, (void *)buffer, (int)len);
  }


