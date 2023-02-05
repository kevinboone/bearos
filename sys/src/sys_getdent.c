/*============================================================================
 *  sys/sys_opendir.c
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

/*============================================================================
 * ==========================================================================*/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pico/stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/direntry.h>
#include <sys/process.h>
#include <sys/error.h>
#include <errno.h>
#include <syslog/syslog.h>
#include <sys/syscalls.h>

#define TRACE SYSLOG_TRACE
#define DEBUG SYSLOG_DEBUG
#define INFO SYSLOG_INFO
#define WARN SYSLOG_WARN

/*=========================================================================
  sys_getdent
=========================================================================*/
int sys_getdent (int fd, DirEntry *de)
  {
  Process *p = process_get_current();
  FileDesc *f;
  Error ret = process_get_filedesc (p, fd, &f);
  if (ret == 0)
    {
    if (f->type == S_IFDIR)
      {
      int n = sys_read (fd, de, sizeof (DirEntry));
      if (n <= 0) 
        ret = 0;
      else
        ret = 1;
      }
    else
      {
      ret = -ENOTDIR;
#ifdef DEBUG
      DEBUG ("Dir operation on non-dir");
#endif
      }
    }
  else
    {
    ret = -EBADF;
#ifdef DEBUG
    DEBUG ("Invalid fd");
#endif
    }
  return ret;
  }

intptr_t _sys_getdent (intptr_t fd, intptr_t de, intptr_t notused)
  {
  (void)notused;
  return sys_getdent (fd, (DirEntry *)de);
  }


