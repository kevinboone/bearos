/*============================================================================
 *  sys/datetime.h
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

#pragma once

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/** year is 4 digits, month is 1 - 12. A positive UTC offset means that the
      local time came from a system that is _ahead_ of UTC, and the result
      is thus _smaller_ than would be the case with a UTC local time. */ 
time_t datetime_ymdhms_to_unix (int year, int month, int day, 
          int hour, int min, int sec, int utc_off_min);

#ifdef __cplusplus
}
#endif




