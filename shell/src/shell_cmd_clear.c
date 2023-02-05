/*============================================================================
 *  shell/shell_cmd_clear.c
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
#include <sys/fsutil.h>
#include <compat/compat.h>
#include <bearos/terminal.h>

/*=========================================================================
  show_usage 
=========================================================================*/
static void show_usage (const char *argv0)
  {
  compat_printf ("Usage: %s [ > device]\n", argv0);
  compat_printf ("Sends a clear/home command to a terminal\n", argv0);
  }

/*=========================================================================
  shell_cmd_clear
=========================================================================*/
Error shell_cmd_clear (int argc, char **argv)
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
    compat_printf (TERM_ESC_HOME TERM_ESC_CLEAR_ALL);
    }
 
  if (usage) ret = 0;
  return ret;
  }




