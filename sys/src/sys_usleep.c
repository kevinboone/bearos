/*============================================================================
 *  sys/sys_usleep.c
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
#include <sys/syscalls.h>

/*============================================================================
 * sys_usleep
 * ==========================================================================*/
intptr_t _sys_usleep (intptr_t usec, intptr_t arg1, intptr_t arg2)
  { 
  (void)arg1; (void)arg2;
  sleep_us ((uint64_t)usec);
  return 0;
  }



