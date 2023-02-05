/*============================================================================
 *  shell/shell_cmd_echo.c
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

/*============================================================================
 * ==========================================================================*/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <getopt.h>
#include <pico/stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/error.h>
#include <term/term.h>
#include <shell/shell.h>
#include <klib/string.h>
#include <sys/syscalls.h>
#include <compat/compat.h>

/*=========================================================================
  show_usage 
=========================================================================*/
static void show_usage (const char *argv0)
  {
  compat_printf ("Usage: %s {-en} {strings...} \n", argv0);
  }

/*=========================================================================
  hextobin 
=========================================================================*/
static unsigned char hextobin (char c)
  {
  switch (c)
    {
    default: return (unsigned char) (c - '0');
    case 'a': case 'A': return 10;
    case 'b': case 'B': return 11;
    case 'c': case 'C': return 12;
    case 'd': case 'D': return 13;
    case 'e': case 'E': return 14;
    case 'f': case 'F': return 15;
    }
  }

/*=========================================================================
  shell_do_arg
=========================================================================*/
static void do_arg (const char *arg)
  {
  char const *s = arg;
  unsigned char c;

  while ((c = (unsigned char) *s++))
    {
    if (c == '\\' && *s)
      {
      switch (c = (unsigned char) *s++)
        {
        case 'a': c = '\a'; break;
        case 'b': c = '\b'; break;
        case 'c': return ; // Not sure how to handle this
        case 'e': c = '\x1B'; break;
        case 'f': c = '\f'; break;
        case 'n': c = '\n'; break;
        case 'r': c = '\r'; break;
        case 't': c = '\t'; break;
        case 'v': c = '\v'; break;
        case 'x':
          {
          char ch = *s;
          if (! isxdigit (ch))
            goto not_an_escape;
          s++;
          c = hextobin (ch);
          ch = *s;
          if (isxdigit (ch))
            {
            s++;
            c = (unsigned char)(c * 16 + hextobin (ch));
            }
          }
          break;
        case '0':
          c = 0;
          if (! ('0' <= *s && *s <= '7'))
            break;
          c = (unsigned char) *s++;
          // fall through
          __attribute__ ((fallthrough));
        case '1': case '2': case '3':
        case '4': case '5': case '6': case '7':
          c -= '0';
          if ('0' <= *s && *s <= '7')
            c = (unsigned char)(c * 8 + (*s++ - '0'));
          if ('0' <= *s && *s <= '7')
            c = (unsigned char) (c * 8 + (*s++ - '0'));
          break;
        case '\\': break;
        not_an_escape:
        default:  compat_printf ("\\"); break;
        }
      }
    compat_printf ("%c", c);
    }
  }

/*=========================================================================
  shell_cmd_echo
=========================================================================*/
Error shell_cmd_echo (int argc, char **argv)
  {
  int ret = 0;
  int opt;
  optind = 0;
  BOOL usage = FALSE;
  BOOL esc = FALSE;
  BOOL skipnl = FALSE;

  while ((opt = getopt (argc, argv, "ehn")) != -1)
    {
    switch (opt)
      {
      case 'e':
        esc = TRUE;
        break;
      case 'n':
        skipnl = TRUE;
        break;
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
    for (int i = optind; i < argc; i++)
      {
      if (esc)
        do_arg (argv[i]);
      else
        compat_printf ("%s", argv[i]);
      if (i != argc - 1) 
        compat_printf (" ");
      }
    }
  
  if (!skipnl)
    compat_printf ("\n");

  if (usage) ret = 0;
  return ret;
  }


