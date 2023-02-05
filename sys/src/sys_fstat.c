/*============================================================================
 *  sys/sys_fstat.c
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
 * sys_fstat 
 * ==========================================================================*/
int sys_fstat (int fd, struct stat *sb)
  {
  TRACE_IN;
#ifdef DEBUG
  DEBUG ("fd=%d", fd);
#endif

  FileDesc *f = NULL;
  Process *p = process_get_current();
  Error ret = process_get_filedesc (p, fd, &f);
  if (ret == 0)
    {
    if (f)
      {
      memset (sb, 0, sizeof (struct stat));
      sb->st_mode = (unsigned int)f->type;
      if (f->get_size) 
        sb->st_size = f->get_size (f);
      else
        sb->st_size = 0;
      if (f->get_mtime)
        sb->st_mtime = f->get_mtime (f);
      else
        sb->st_mtime = 0;
      }
    else
      ret = EBADF;
    } 

  TRACE_OUT;
  return ret;
  }

intptr_t _sys_fstat (intptr_t fd, intptr_t sb, intptr_t notused)
  {
  (void)notused;
  return sys_fstat ((int)fd, (struct stat *)sb);
  }




