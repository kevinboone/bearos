/*============================================================================
 *  sys/sys_localtime_r.c
 *
 * ==========================================================================*/

/*============================================================================
 * ==========================================================================*/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <pico/stdlib.h>
#include <stdio.h>
#include <sys/process.h>
#include <sys/syscalls.h>

/*=========================================================================
  sys_localtime_r
=========================================================================*/
struct tm *sys_localtime_r (time_t t, struct tm *tm)
  {
  Process * p = process_get_current();
  return sys_gmtime_r (t + process_get_utc_offset (p) * 60, tm);
  }

intptr_t _sys_localtime_r (intptr_t t, intptr_t tm, intptr_t notused)
  {
  (void)notused;
  return (intptr_t) sys_localtime_r (t, (struct tm *)tm);
  }





