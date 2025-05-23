/*============================================================================
 *  sys/strings.c
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/diskinfo.h>
#include <syslog/syslog.h>
#include <sys/syscalls.h>
#include <sys/filedesc.h>
#include <sys/process.h>

#define TRACE_IN SYSLOG_TRACE_IN
#define TRACE_OUT SYSLOG_TRACE_OUT
#define TRACE SYSLOG_TRACE
#define DEBUG SYSLOG_DEBUG
#define INFO SYSLOG_INFO
#define WARN SYSLOG_WARN

/*============================================================================
 * itoa
 * ==========================================================================*/
int32_t _sys_itoa (int32_t number, int32_t buffer, int32_t base)
  {
#if PICO_ON_DEVICE
  return (int32_t) itoa (number, (char *)buffer, base);
#else
  return compat_itoa (number, buffer, base);
#endif
  }

/*============================================================================
 * atoi
 * ==========================================================================*/
int32_t _sys_atoi (int32_t buffer, int32_t notused1, int32_t notused2)
  {
  (void)notused1; (void)notused2;
#if PICO_ON_DEVICE
  return (int32_t) atoi ((const char *)buffer);
#else
  return atoi (buffer);
#endif
  }

/*============================================================================
 * strcmp
 * ==========================================================================*/
int32_t _sys_strcmp (int32_t s1, int32_t s2, int32_t unused)
  {
  (void)unused; 
  return (int32_t) strcmp ((const char *)s1, (const char *)s2);
  }

/*============================================================================
 * strncmp
 * ==========================================================================*/
int32_t _sys_strncmp (int32_t s1, int32_t s2, int32_t n)
  {
  return (int32_t) strncmp ((const char *)s1, (const char *)s2, (size_t)n);
  }

/*============================================================================
 * strlen 
 * ==========================================================================*/
int32_t _sys_strlen (int32_t str, int32_t unused1, int32_t unused2)
  {
  (void)unused1; (void)unused2;
  return (int32_t) strlen ((const char *)str);
  }

/*============================================================================
 * strtol 
 * ==========================================================================*/
int32_t _sys_strtol (int32_t buffer, int32_t end, int32_t base)
  {
  return (int32_t)strtol ((char *)buffer, (char **)end, base);
  }


