/*============================================================================
 *  sys/time.c
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/diskinfo.h>
#include <sys/syscalls.h>
#include <sys/clocks.h>

/*============================================================================
 * _sys_gettimeofday
 * ==========================================================================*/
intptr_t _sys_gettimeofday (intptr_t tv, intptr_t tz, intptr_t notused)
  {
  (void)notused; (void)tz;

  // tz is normally 0 -- applications don't usually adjust timezones
  //   this way
  if (tv)
    {
    ((struct timeval *)tv)->tv_sec = clocks_get_time();
    ((struct timeval *)tv)->tv_usec = 0; 
    }

  return 0; 
  }

