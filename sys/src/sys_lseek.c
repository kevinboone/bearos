/*============================================================================
 *  sys/sys_lseek.c
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
 * sys_lseek
 * ==========================================================================*/
int32_t sys_lseek (int fd, int32_t offset, int origin)
  {
  TRACE_IN;
#ifdef DEBUG
    DEBUG ("fd=%d off=%d orig=%d", fd, offset, origin);
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
    WARN ("Seeking fd that is not open: %d", fd);
    return -EINVAL;
    }
  
  int ret;

  if (filedesc->lseek)
    ret = filedesc->lseek (filedesc, offset, origin);
  else
    ret = -ENOSYS;

  TRACE_OUT;
  return ret;
  }

intptr_t _sys_lseek (intptr_t fd, intptr_t offset, intptr_t origin)
  {
  return sys_lseek (fd, offset, origin);
  }




