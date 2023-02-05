/*============================================================================
 *  sys/sys_open.c
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/error.h>
#include <errno.h>
#include <fcntl.h>
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
 * open 
 * ==========================================================================*/
int sys_open (const char *path, int flags)
  {
  TRACE_IN;
#ifdef DEBUG
    DEBUG ("path=%s, flags=%04X", path, flags);
#endif
  Process *p = process_get_current();

  int fd = -EINVAL;
  Error ret = process_get_next_fd (p, &fd);
  if (ret)
    {
    WARN ("Failed to get FD: error %d", ret);
    return -ENFILE;
    }
  if (!path[0])
    {
    WARN ("Empty path");
    return -EINVAL;
    }

  char abspath[PATH_MAX];
  fsutil_make_abs_path (path, abspath, PATH_MAX);

  FSysDescriptor *desc = fsmanager_get_descriptor_by_path (abspath);
  if (desc)
    {
    Error error;
    FileDesc *filedesc = desc->get_filedesc (desc, abspath + 2, flags, &error);
    if (error == 0)
      {
      if (filedesc->type == S_IFDIR && flags != O_RDONLY)
        {
        filedesc->close (filedesc);
        //filedesc_destroy (filedesc);
        fd = -EISDIR;
        }
      else
        process_set_filedesc (p, fd, filedesc);
      }
    else
      {
      fd = -error;
      }
    }
  else
   return -ENOENT;

  TRACE_OUT;
  return fd;
  }


intptr_t _sys_open (intptr_t path, intptr_t flags, intptr_t notused)
  {
  (void)notused;
  return sys_open ((const char *)path, (int)flags);
  }


