/*============================================================================
 *  compat/console.c
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

/*============================================================================
 * ==========================================================================*/
#define _GNU_SOURCE

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <pico/stdlib.h>
#include <sys/syscalls.h>
#include <compat/compat.h>

/*=========================================================================
  compat_printf
=========================================================================*/
void compat_printf (const char *fmt, ...)
  {
  va_list ap;
  va_start (ap, fmt);
  char *s;
  int n = vasprintf (&s, fmt, ap);
  sys_write (1, s, n); 
  free (s);
  va_end (ap);
  }

/*=========================================================================
  compat_printf_stderr
=========================================================================*/
void compat_printf_stderr (const char *fmt, ...)
  {
  va_list ap;
  va_start (ap, fmt);
  char *s;
  int n = vasprintf (&s, fmt, ap);
  sys_write (2, s, n); 
  free (s);
  va_end (ap);
  }



