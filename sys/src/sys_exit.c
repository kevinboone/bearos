/*============================================================================
 *  sys/sys_exit.c
 
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

/*============================================================================
 * ==========================================================================*/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
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

jmp_buf foo;

/*=========================================================================
  _sys_exit
=========================================================================*/
intptr_t _sys_exit (intptr_t status, intptr_t notused1, intptr_t notused2)
  {
  (void)notused1; (void)notused2;
  Process *p = process_get_current();
  longjmp (p->exit_jmp, status);
  return 0;
  }


