/*============================================================================
 *  sys/sys_close.c
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
 * close 
 * ==========================================================================*/
int sys_close (int fd)
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
    WARN ("Closing fd that is not open: %d", fd);
    return EINVAL;
    }

  filedesc->close (filedesc);
  process_set_filedesc (p, fd, 0);

  TRACE_OUT;
  return 0;
  }


intptr_t _sys_close (intptr_t fd, intptr_t notused1, intptr_t notused2)
  { 
  (void)notused1; (void)notused2;
  return sys_close (fd);
  }

