/*============================================================================
 *  sys/sys_write.c
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/error.h>
#include <errno.h>
#include <sys/syscalls.h>
#include <sys/filedesc.h>
#include <sys/process.h>

/*============================================================================
 * sys_write 
 * ==========================================================================*/
int sys_write (int fd, const void *buffer, int len)
  {
  Process *p = process_get_current();

  if (fd < 0 || fd >= NFILES)
    return -EINVAL;

  FileDesc *filedesc = 0;
  process_get_filedesc (p, fd, &filedesc);

  if (!filedesc)
    return -EINVAL;
  
  Error ret = filedesc->write (filedesc, buffer, len);

  return ret;
  }

intptr_t _sys_write (intptr_t fd, intptr_t buffer, intptr_t len)
  {
  return sys_write ((int)fd, (void *)buffer, (int)len);
  }




