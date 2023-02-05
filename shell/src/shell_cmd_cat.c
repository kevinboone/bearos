/*============================================================================
 *  shell/shell_cmd_cat.c
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

/*=========================================================================
  do_cat 
=========================================================================*/
static Error do_cat (int fd, BOOL unbuffered)
  {
  char buff[256];
  int bufsize = unbuffered ? 1 : sizeof (buff);
  int n;
  while ((n = sys_read (fd, buff, bufsize)) > 0)
    sys_write (1, buff, n);

  return 0;
  }

/*=========================================================================
  show_usage 
=========================================================================*/
static void show_usage (const char *argv0)
  {
  compat_printf ("Usage: %s [files...]\n", argv0);
  compat_printf ("If no files are specified, read from stdin\n", argv0);
  }

/*=========================================================================
  shell_cmd_cat
=========================================================================*/
Error shell_cmd_cat (int argc, char **argv)
  {
  int ret = 0;
  BOOL usage = FALSE;
  BOOL unbuffered = FALSE;

  optind = 0;
  int opt;
  while ((opt = getopt (argc, argv, "uh")) != -1)
    {
    switch (opt)
      {
      case 'u':
        unbuffered = TRUE;
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
    if (argc == optind)
      {
      ret = do_cat (0, unbuffered); // STDIN
      }
    else
      {
      for (int i = optind; i < argc && ret == 0; i++)
        {
        if (fsutil_is_directory (argv[i]))
          {
          ret = EISDIR;
	  compat_printf_stderr ("%s: %s: %s\n", argv[0], argv[i], 
            strerror (ret));
          }
        else
          {
	  int fd = sys_open (argv[i], O_RDONLY);
	  if (fd >= 0)
	    {
	    ret = do_cat (fd, unbuffered); 
	    sys_close (fd);
	    }
	  else
	    {
	    ret = -fd;
	    compat_printf_stderr ("%s: %s: %s\n", argv[0], argv[i], 
               strerror (ret));
            }
	  }
        }
      }
    }
  
  if (usage) ret = 0;
  return ret;
  }




