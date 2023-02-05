/*============================================================================
 *  shell/shell_cmd_uname.c
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
#include <sys/stat.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/error.h>
#include <errno.h>
#include <sys/fsutil.h>
#include <term/term.h>
#include <shell/shell.h>
#include <klib/string.h>
#include <sys/syscalls.h>
#include <sys/direntry.h>
#include <compat/compat.h>
#include "../../version.h" 

/*=========================================================================
  show_usage 
=========================================================================*/
static void show_usage (const char *argv0)
  {
  compat_printf ("Usage: %s\n", argv0);
  compat_printf ("Print system information.\n", argv0);
  }

/*=========================================================================
  shell_cmd_uname
=========================================================================*/
Error shell_cmd_uname (int argc, char **argv)
  {
  Error ret = 0;
  int opt;
  optind = 0;
  BOOL usage = FALSE;

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
    compat_printf ("%s Version %d.%d.%d %s\n", BEAROS_PLATFORM_NAME, 
        BEAROS_MAJOR_VERSION, BEAROS_MINOR_VERSION, BEAROS_MICRO_VERSION, 
        __DATE__);
    }

  if (usage) ret = 0;
  return ret;
  }



