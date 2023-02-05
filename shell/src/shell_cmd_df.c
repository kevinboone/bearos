/*============================================================================
 *  shell/shell_cmd_df.c
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
#include <sys/error.h>
#include <errno.h>
#include <shell/shell.h>
#include <klib/string.h>
#include <sys/diskinfo.h>
#include <compat/compat.h>


/*=========================================================================
  do_drive 
=========================================================================*/
static void do_drive (int8_t drive, const DiskInfo *di, bool kilo)
  {
  if (kilo)
    compat_printf ("%c: %ldk %ldk %s\n", drive + 'A', di->total_sectors / 2, 
	  di->free_sectors / 2, di->name);
  else
    compat_printf ("%c: %ld %ld %s\n", drive + 'A', di->total_sectors, 
	  di->free_sectors, di->name);
  }

/*=========================================================================
  usage 
=========================================================================*/
static void show_usage (const char *argv0)
  {
  compat_printf ("Usage: %s [drive or path]\n", argv0);
  }

/*=========================================================================
  shell_cmd_df
=========================================================================*/
Error shell_cmd_df (int argc, char **argv)
  {
  Error ret = 0;
  DiskInfo diskinfo;

  int opt;
  optind = 0;
  BOOL usage = FALSE;
  BOOL kilo = FALSE;

  while ((opt = getopt (argc, argv, "hk")) != -1)
    {
    switch (opt)
      {
      case 'k':
        kilo = TRUE;
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
    if (argc - optind >= 1)
      {
      int d = toupper (argv[optind][0]);
      d -= 'A'; 
      if (d < 0 || d >= FSMANAGER_MAX_MOUNTS)
	{
	ret = ENOENT; 
	compat_printf_stderr ("%s: %s\n", argv[0], strerror (ret));
	}
      else
	{
	Error ret = sys_get_diskinfo ((int8_t)d, &diskinfo);
	if (ret == 0)
	  {
	  do_drive ((int8_t)d, &diskinfo, kilo);
	  } 
	else
	  {
	  compat_printf_stderr ("%s: %s\n", argv[0], strerror (ret));
	  }
	}
      }
    else
      {
      for (int8_t i = 0; i < FSMANAGER_MAX_MOUNTS; i++)
	{
	Error ret = sys_get_diskinfo (i, &diskinfo);
	if (ret == 0)
	  {
	  do_drive (i, &diskinfo, kilo);
	  } 
	}
      }
    }

  if (usage) ret = 0;
  return ret;
  }

