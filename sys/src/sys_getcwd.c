/*============================================================================
 *  sys/sys_getcwd.c
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
  sys_getcwd
=========================================================================*/
int sys_getcwd (char *dir, int len)
  {
  Process *p = process_get_current();
  strncpy (dir, process_get_cwd (p), (size_t)len);
  dir[len - 1] = 0;
  return 0;
  }

intptr_t _sys_getcwd (intptr_t path, intptr_t size, intptr_t notused)
  {
  (void)notused;
  return sys_getcwd ((char *)path, (int)size);
  }



