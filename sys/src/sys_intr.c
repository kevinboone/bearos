/*============================================================================
 *  sys/sys_intr.c
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

#include <stdint.h>
#include <stdlib.h>
#include <pico/stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/error.h>
#include <errno.h>
#include <sys/diskinfo.h>
#include <syslog/syslog.h>
#include <sys/syscalls.h>
#include <sys/process.h>
#include <term/term.h>
#include <bearos/terminal.h>
#include <bearos/intr.h>

#define TRACE_IN SYSLOG_TRACE_IN
#define TRACE_OUT SYSLOG_TRACE_OUT
#define TRACE SYSLOG_TRACE
#define DEBUG SYSLOG_DEBUG
#define INFO SYSLOG_INFO
#define WARN SYSLOG_WARN

static bool intr_term = false;

/*============================================================================
 * sys_poll_interrupt 
 * ==========================================================================*/
int sys_poll_interrupt (int intr_num)
  {
  if (intr_num == SYS_INTR_TERM)
    {
    Process *p = process_get_current();
    FileDesc *f = p->files[0];
    if (f)
      {
      if (f->read_timeout)
        {
        int c = f->read_timeout (f, 0);
        if (c == I_INTR)
          {
          intr_term = true;
          return 1;
          }
        }
      }
    }
  return 0;
  }

intptr_t _sys_poll_interrupt (intptr_t intr_num, intptr_t notused1, 
     intptr_t notused2)
  {
  (void)notused1; (void)notused2;
  return sys_poll_interrupt (intr_num);
  }

/*============================================================================
 * sys_clear_interrupt 
 * ==========================================================================*/
int sys_clear_interrupt (int intr_num)
  {
  if (intr_num == SYS_INTR_TERM)
    {
    intr_term = false;
    }
  return 0;
  }

intptr_t _sys_clear_interrupt (intptr_t intr_num, intptr_t notused1, 
     intptr_t notused2)
  {
  (void)notused1; (void)notused2;
  return sys_clear_interrupt ((int)intr_num);
  }


