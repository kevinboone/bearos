/*============================================================================
 *  sys/clocks.c
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/error.h>
#include <time.h>
#include <errno.h>
#include <syslog/syslog.h>
#include <sys/clocks.h>
#include <ds3231/ds3231.h>
#include <sys/datetime.h>
#include <sys/process.h>
#include <sys/syscalls.h>
#if PICO_ON_DEVICE
#include <hardware/rtc.h>
#include <pico/types.h>
#endif

#define TRACE_IN SYSLOG_TRACE_IN
#define TRACE_OUT SYSLOG_TRACE_OUT
#define TRACE SYSLOG_TRACE
#define DEBUG SYSLOG_DEBUG
#define INFO SYSLOG_INFO
#define WARN SYSLOG_WARN

DS3231 *ds3231_inst;

/*============================================================================
 * clocks_get_time 
 * ==========================================================================*/
time_t clocks_get_time (void)
  {
#if PICO_ON_DEVICE
  datetime_t dt;
  rtc_get_datetime (&dt);
  //printf ("get date %d\n", dt.day);

  time_t t = datetime_ymdhms_to_unix (dt.year, dt.month, dt.day, 
          dt.hour, dt.min, dt.sec, 0); // UTC offset zero, since we are using
#else
  time_t t = time (NULL);
#endif

  return t;
  }

/*============================================================================
 * clocks_get_time 
 * ==========================================================================*/
Error clocks_set_time (time_t time)
  {
#if PICO_ON_DEVICE
  struct tm tm;
  sys_gmtime_r (time, &tm);
  datetime_t dt;
  dt.year = (int16_t)tm.tm_year + 1900;
  dt.month = (int8_t)(tm.tm_mon + 1);
  dt.day = (int8_t)(tm.tm_mday);
  // The Pico RTC does not actually use of calculate the day-of-week field.
  // But if you don't write a value 0-6 for this field, setting the date
  //   fails. It took me hours to figure this out.
  dt.dotw = 0;
  dt.hour = (int8_t)(tm.tm_hour);
  dt.min = (int8_t)(tm.tm_min);
  dt.sec = (int8_t)(tm.tm_sec);
  //printf ("setting %d %d %d %d:%d:%d\n", 
  //   dt.year, dt.month, dt.day, dt.hour, dt.min, dt.sec);
  rtc_set_datetime (&dt);

  ds3231_set_datetime (ds3231_inst, dt.year, dt.month, dt.day, dt.hour,
     dt.min, dt.sec);
#else
  (void)time;
#endif
  return 0;
  }

/*============================================================================
 * clocks_harware_to_system
 * ==========================================================================*/
void clocks_hardware_to_system (void)
  {
#if PICO_ON_DEVICE
  int year, month, day, hour, min, sec;

  ds3231_get_datetime (ds3231_inst, &year, &month, &day, &hour, &min, &sec); 

  datetime_t dt = {
    .year = (int16_t)year,
    .month = (int8_t)month,
    .day = (int8_t)day,
    .hour = (int8_t)hour,
    .min = (int8_t)min,
    .sec = (int8_t)sec
  };

#ifdef INFO
  INFO ("Synchronizing clocks; %d %d %d %d:%d:%d\n", dt.year, dt.month, dt.day, dt.hour, dt.min, dt.sec);
#endif

  rtc_set_datetime (&dt);
#endif
  }


