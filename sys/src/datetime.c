/*============================================================================
 *  sys/datetime.c
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/datetime.h>

/*============================================================================
 *  
 * ==========================================================================*/
// Starting DOY for each month (not allowing for leap years)
static int month_offsets[] = {0,31,59,90,120,151,181,212,243,273,304,334};	

/*============================================================================
 * datetime_ymdhms_to_unix 
 * ==========================================================================*/
time_t datetime_ymdhms_to_unix (int year, int month, int day, 
          int hour, int min, int sec, int utc_off_min)
  {
  if (month > 12 || month < 1) return 0; // Out of bounds will crash

/*
  printf ("year=%d\n", year);
  printf ("month=%d\n", month);
  printf ("day=%d\n", day);
  printf ("hour=%d\n", hour);
  printf ("min=%d\n", min);
  printf ("sec=%d\n", sec);
*/

  int leap_year_count = year / 4;
  int epoch_day_number = month_offsets [month - 1];
  if ((year % 4 == 0) && (year % 100 != 0))
    { 
    if (epoch_day_number < 32) 
      { 
      epoch_day_number--; 
      }
    }
  epoch_day_number = (year - 1970) * 365 + leap_year_count 
          + epoch_day_number + day - 493;

  unsigned int time_of_day_secs = (unsigned int)(hour * 3600) + 
      (unsigned int)((min - utc_off_min) * 60) + (unsigned int)sec;

  return (unsigned int)epoch_day_number * 
      (unsigned int)86400 + (unsigned int)time_of_day_secs; 
  }

