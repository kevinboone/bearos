/*============================================================================
 *  sys/sys_chdir.c
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
#include <fcntl.h>
#include <syslog/syslog.h>
#include <sys/syscalls.h>

#define TRACE SYSLOG_TRACE
#define DEBUG SYSLOG_DEBUG
#define INFO SYSLOG_INFO
#define WARN SYSLOG_WARN

/*=========================================================================
  sys_chdir
=========================================================================*/
int sys_chdir (const char *dir)
  {
  return process_set_cwd (process_get_current(), dir);
  }

intptr_t _sys_chdir (intptr_t path, intptr_t notused1, intptr_t notused2)
  {
  (void)notused1; (void)notused2;
  return sys_chdir ((const char *)path);
  }


