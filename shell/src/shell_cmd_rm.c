/*============================================================================
 *  shell/shell_cmd_rm.c
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
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syscalls.h>
#include <fcntl.h>
#include <sys/error.h>
#include <sys/fsutil.h>
#include <errno.h>
#include <shell/shell.h>
#include <klib/string.h>
#include <compat/compat.h>

/*=========================================================================
  show_usage 
=========================================================================*/
static void show_usage (const char *argv0)
  {
  compat_printf ("Usage: %s {files...}\n", argv0);
  }

/*=========================================================================
  rm_path 
=========================================================================*/
static Error rm_path (const char *argv0, const char *path, bool verbose)
  {
  Error ret = 0;
  if (verbose) compat_printf ("Deleting '%s'\n", path);
  if (fsutil_is_directory (path))
    {
    compat_printf_stderr ("Not deleting directory '%s'\n", path);
    ret = EINVAL;
    }
  else
    {
    Error ret = sys_unlink (path);
    if (ret)
      compat_printf_stderr ("%s: %s\n", argv0, strerror (ret));
    }
  return ret;
  }

/*=========================================================================
  shell_cmd_rm
=========================================================================*/
Error shell_cmd_rm (int argc, char **argv)
  {
  int opt;
  Error ret = 0;
  optind = 0;
  BOOL usage = FALSE;
  BOOL verbose = FALSE;
  while ((opt = getopt (argc, argv, "hv")) != -1)
    {
    switch (opt)
      {
      case 'v':
        verbose = TRUE;
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
    if (argc == 1)
      {
      show_usage (argv[0]);
      ret = EINVAL;
      }
    else
      {
      for (int i = optind; i < argc; i++)
        {
        const char *path = argv[i];
        ret = rm_path (argv[0], path, verbose);
        }
      }
    }

  if (usage) ret = 0;
  return ret;
  }

