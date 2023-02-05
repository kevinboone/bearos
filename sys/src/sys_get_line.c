/*============================================================================
 *  sys/sys_get_line.c
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <utime.h>
#include <sys/error.h>
#include <sys/fsmanager.h>
#include <sys/fsutil.h>
#include <errno.h>
#include <syslog/syslog.h>
#include <sys/syscalls.h>
#include <term/term.h>

/*============================================================================
 * _sys_get_line
 * ==========================================================================*/
intptr_t _sys_get_line (intptr_t fd_in, intptr_t buff, intptr_t len)
  {
  return (intptr_t) term_get_line ((int)fd_in, (char *)buff, (int)len,
          0, NULL);
  }



