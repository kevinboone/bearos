/*============================================================================
 *  compat/itoa.c
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

/*============================================================================
 * ==========================================================================*/

#include <stdint.h>
#include <pico/stdlib.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <compat/compat.h>

/*==========================================================================
  compat_itoa
==========================================================================*/
#if PICO_ON_DEVICE
#else
static void swap (char *x, char *y) { char t = *x; *x = *y; *y = t; }

static void reverse(char *buffer, int i, int j)
  {
  while (i < j)
    swap (&buffer[i++], &buffer[j--]);
  }

char *compat_itoa (int num, char* str, int base)
  {
  int i = 0;
  bool isNegative = false;

  if (num == 0)
    {
    str[i++] = '0';
    str[i] = '\0';
    return str;
    }

  if (num < 0 && base == 10)
    {
    isNegative = true;
    num = -num;
    }

  while (num != 0)
    {
    int rem = num % base;
    str[i++] = (char) ((rem > 9)? (rem-10) + 'a' : rem + '0' );
    num = num/base;
    }

  if (isNegative)
    str[i++] = '-';

  str[i] = '\0'; 

  reverse (str, 0, i - 1);
  return str;
  }
#endif

