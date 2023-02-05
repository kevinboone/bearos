/*============================================================================
 *  sys/sys_gmtime_r.c
 *
 * This implementation is derived from the one in MUSL libc, which is
 *   distributed under MIT open source licencing terms. 
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

#define DAYS_PER_400Y (365*400 + 97)
#define DAYS_PER_100Y (365*100 + 24)
#define DAYS_PER_4Y   (365*4   + 1)
#define LEAPOCH       (946684800LL + 86400*(31+29))

/*=========================================================================
  sys_gmtime_r
=========================================================================*/
struct tm *sys_gmtime_r (time_t t, struct tm *tm)
  {
  long long days, secs;
  int remdays, remsecs, remyears;
  int qc_cycles, c_cycles, q_cycles;
  int years, months;
  int wday, yday, leap;
  static const char days_in_month[] = {31,30,31,30,31,31,30,31,30,31,31,29};

  secs = t - LEAPOCH;
  days = secs / 86400;
  remsecs = (int)(secs % 86400);
  if (remsecs < 0) 
    {
    remsecs += 86400;
    days--;
    }

  wday = (int)(3+days)%7;
  if (wday < 0) wday += 7;

  qc_cycles = (int)(days / DAYS_PER_400Y);
  remdays = (int) (days % DAYS_PER_400Y);
  if (remdays < 0) 
    {
    remdays += DAYS_PER_400Y;
    qc_cycles--;
    }

  c_cycles = remdays / DAYS_PER_100Y;
  if (c_cycles == 4) c_cycles--;
  remdays -= c_cycles * DAYS_PER_100Y;

  q_cycles = remdays / DAYS_PER_4Y;
  if (q_cycles == 25) q_cycles--;
  remdays -= q_cycles * DAYS_PER_4Y;

  remyears = remdays / 365;
  if (remyears == 4) remyears--;
  remdays -= remyears * 365;

  leap = !remyears && (q_cycles || !c_cycles);
  yday = remdays + 31 + 28 + leap;
  if (yday >= 365+leap) yday -= 365+leap;

  years = remyears + 4*q_cycles + 100*c_cycles + 400*qc_cycles;

  for (months=0; days_in_month[months] <= remdays; months++)
    remdays -= days_in_month[months];

  tm->tm_year = years + 100;
  tm->tm_mon = months + 2;
  if (tm->tm_mon >= 12) 
    {
    tm->tm_mon -=12;
    tm->tm_year++;
    }
  tm->tm_mday = remdays + 1;
  tm->tm_wday = wday;
  tm->tm_yday = yday;

  tm->tm_hour = remsecs / 3600;
  tm->tm_min = remsecs / 60 % 60;
  tm->tm_sec = remsecs % 60;

  return tm;
  }

intptr_t _sys_gmtime_r (intptr_t t, intptr_t tm, intptr_t notused)
  {
  (void)notused;
  return (intptr_t) sys_gmtime_r (t, (struct tm *)tm);
  }


