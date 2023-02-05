/*============================================================================
 * syslog/syslog.c
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 *
 * ==========================================================================*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <syslog/syslog.h>

static SyslogLevel level = LOGLEVEL_WARN;

void syslog (SyslogLevel _level, const char *msg, ...)
  {
  if (_level > level) return;
  char str[300];
  va_list argList;
  va_start (argList, msg);
  vsnprintf (str, sizeof(str), msg, argList);
  va_end (argList);
  printf ("%d %s\n", level, str);
  }

void syslog_trace (const char *func, int line, const char *msg, ...)
  {
  char str[300];
  char str2[350];
  va_list argList;
  va_start (argList, msg);
  vsnprintf (str, sizeof(str), msg, argList);
  va_end (argList);
  sprintf (str2, "%s:%d %s", func, line, str);
  syslog (LOGLEVEL_TRACE, str2);
  }

void syslog_debug (const char *func, int line, const char *msg, ...)
  {
  char str[300];
  char str2[350];
  va_list argList;
  va_start (argList, msg);
  vsnprintf (str, sizeof(str), msg, argList);
  va_end (argList);
  sprintf (str2, "%s:%d %s", func, line, str);
  syslog (LOGLEVEL_DEBUG, str2);
  }

void syslog_info (const char *func, int line, const char *msg, ...)
  {
  char str[300];
  char str2[350];
  va_list argList;
  va_start (argList, msg);
  vsnprintf (str, sizeof(str), msg, argList);
  va_end (argList);
  sprintf (str2, "%s:%d %s", func, line, str);
  syslog (LOGLEVEL_INFO, str2);
  }

void syslog_warn (const char *func, int line, const char *msg, ...)
  {
  char str[300];
  char str2[350];
  va_list argList;
  va_start (argList, msg);
  vsnprintf (str, sizeof(str), msg, argList);
  va_end (argList);
  sprintf (str2, "%s:%d %s", func, line, str);
  syslog (LOGLEVEL_WARN, str2);
  }

void syslog_error (const char *func, int line, const char *msg, ...)
  {
  char str[300];
  char str2[350];
  va_list argList;
  va_start (argList, msg);
  vsnprintf (str, sizeof(str), msg, argList);
  va_end (argList);
  sprintf (str2, "%s:%d %s", func, line, str);
  syslog (LOGLEVEL_ERROR, str2);
  }

void syslog_set_level (SyslogLevel _level)
  {
  level = _level;
  }


