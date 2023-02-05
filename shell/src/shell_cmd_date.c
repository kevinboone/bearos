/*============================================================================
 *  shell/shell_cmd_date.c
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

/*============================================================================
 * ==========================================================================*/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pico/stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/error.h>
#include <errno.h>
#include <getopt.h>
#include <term/term.h>
#include <shell/shell.h>
#include <klib/string.h>
#include <sys/syscalls.h>
#include <sys/clocks.h>
#include <compat/compat.h>

/*=========================================================================
  show_usage 
=========================================================================*/
static void show_usage (const char *argv0)
  {
  compat_printf ("Usage: %s [MMDDhhmm[CC][YY][ss]]\n", argv0);
  compat_printf ("With no argument, displays the current time and date\n");
  }

/*=========================================================================
  get_number 
=========================================================================*/
static int get_number (const char *spec, int start)
  {
  char s[3];
  s[0] = spec[start];
  s[1] = spec[start + 1];
  s[2] = 0;
  return atoi (s);
  }

/*=========================================================================
  set_time 
=========================================================================*/
static Error set_time (int year, int month, int day, 
       int hour, int min, int sec)
  {
  Error ret = 0;
  struct tm tm = 
    {
    .tm_sec = sec,
    .tm_min = min,
    .tm_hour = hour,
    .tm_mday = day,
    .tm_mon = month - 1,
    .tm_year = year - 1900
    };
  // mktime is supposed to work with _local_ time. However, in the Pico
  //   SDK, lacking timezone specification, it seems to use UTC. So
  //   we must subtract our UTC offset to get the epoch time that would
  //   have been in effect, had the converstion been from local time.
  time_t t = mktime (&tm);
  Process *p = process_get_current();
  int utc_offset_min = process_get_utc_offset (p);
  t = t - utc_offset_min * 60;
  clocks_set_time (t);
  return ret;
  }

/*=========================================================================
  check_range 
=========================================================================*/
static bool check_range (const char *name, int v, int min, int max)
  {
  if (v > max || v < min)
    {
    compat_printf_stderr ("Out of range: %s (%d to %d)\n", name, min, max);
    return false;
    }
  return true;
  }

/*=========================================================================
  shell_cmd_date
=========================================================================*/
static Error set_time_from_spec (const char *argv0, const char *spec)
  {
  Error ret = 0;
  int l = (int)strlen (spec);

  struct tm tml;
  time_t t = clocks_get_time();
  sys_localtime_r (t, &tml); 
  int year = tml.tm_year + 1900;

  if (l == 8 || l == 10 || l == 12 || l == 14)
    {
    int month = get_number (spec, 0);
    if (!check_range ("month", month, 1, 12))
      goto date_range_err;
    int day = get_number (spec, 2);
    if (!check_range ("day", day, 1, 31))
      goto date_range_err;
    int hour = get_number (spec, 4);
    if (!check_range ("hour", hour, 0, 59))
      goto date_range_err;
    int min = get_number (spec, 6);
    if (!check_range ("minute", min, 0, 59))
      goto date_range_err;
    int century = 20;
    int sec = 0;
    if (l >= 14)
      {
      century = get_number (spec, 8);
      year = get_number (spec, 10);
      sec = get_number (spec, 12);
      year = century * 100 + year;
      }
    else if (l >= 12)
      {
      // Eentered a 4-digit year
      century = get_number (spec, 8);
      year = get_number (spec, 10);
      year = century * 100 + year;
      }
    else if (l >= 10)
      {
      // Only entered a 2-digit year
      year = get_number (spec, 8);
      year = century * 100 + year;
      }
    if (!check_range ("second", sec, 0, 59))
      goto date_range_err;
        
    ret = set_time (year, month, day, hour, min, sec);
    }
  else
    {
    compat_printf_stderr ("%s: incorrect date format\n", argv0);
    ret = EINVAL;
    }
  return ret;

date_range_err:
  return EINVAL;
  }

/*=========================================================================
  shell_cmd_date
=========================================================================*/
Error shell_cmd_date (int argc, char **argv)
  {
  int ret = 0;
  BOOL usage = FALSE;

  optind = 0;
  int opt;
  while ((opt = getopt (argc, argv, "h")) != -1)
    {
    switch (opt)
      {
      case 'h':
        usage = TRUE;
        // Fall through
      default:
        show_usage (argv[0]);
        ret = EINVAL;
      }
    }

  if (ret == 0)
    {
    if (argc - optind >= 1)
      {
      ret = set_time_from_spec (argv[0], argv[optind]);
      }
    else
      {
      struct tm tml;
      time_t t = clocks_get_time();
      sys_localtime_r (t, &tml); 
      compat_printf (asctime (&tml));
      }
    }
  
  if (usage) ret = 0;
  return ret;
  }





