/*============================================================================
 *  sys/sys_sbrk.c
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/error.h>
#include <errno.h>
#include <syslog/syslog.h>
#include <sys/syscalls.h>
#include <sys/process.h>

#define TRACE_IN SYSLOG_TRACE_IN
#define TRACE_OUT SYSLOG_TRACE_OUT
#define TRACE SYSLOG_TRACE
#define DEBUG SYSLOG_DEBUG
#define INFO SYSLOG_INFO
#define WARN SYSLOG_WARN

/*============================================================================
 * sys_sbrk
 * ==========================================================================*/
intptr_t _sys_sbrk (intptr_t increment, intptr_t notused1, intptr_t notused2)
  {
  (void)notused1; (void)notused2;

  //printf ("inc=%ld\n", increment);

  Process *p = process_get_current();
  void *old_brk = process_get_break (p);

  //printf ("old=%p\n", old_brk);

  if (increment == 0)
    return (intptr_t)old_brk;

  intptr_t new_brk = (intptr_t)old_brk + increment;
  //printf ("new=%08lX\n", new_brk);

  if (new_brk >= RAMTOP) return -1;
  //printf ("OK\n");

  process_set_break (p, (void *)new_brk);
  return (intptr_t)old_brk;
  }

