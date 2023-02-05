/*============================================================================
 *  shell/shell_cmd_cp_mv.c
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
#include <utime.h>
#include <sys/types.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/error.h>
#include <errno.h>
#include <term/term.h>
#include <shell/shell.h>
#include <shell/shell_cmd.h>
#include <klib/string.h>
#include <sys/syscalls.h>
#include <compat/compat.h>
#include <sys/fsutil.h>

/*=========================================================================
  show_usage 
=========================================================================*/
static void show_usage (const char *argv0)
  {
  compat_printf ("Usage: %s [options] {files...} {file or directory}\n", 
     argv0);
  compat_printf ("    -v     verbose\n");
  compat_printf ("    -p     preserve attributes\n");
  }

/*=========================================================================
  do_move_file 
=========================================================================*/
static void do_move_file (const char *argv0, const char *source, 
         const char *target, bool verbose)
  {
  if (verbose) compat_printf ("%s -> %s\n", source, target);
  Error ret = sys_rename (source, target);
  if (ret)
    {
    compat_printf ("%s: %s\n", argv0, strerror (ret));
    }
  }

/*=========================================================================
  do_copy_file 
=========================================================================*/
static void do_copy_file (const char *argv0, const char *source, 
         const char *target, bool verbose, bool attributes)
  {
  char real_source[PATH_MAX];
  char real_target[PATH_MAX];
  fsutil_make_abs_path (source, real_source, PATH_MAX);
  fsutil_make_abs_path (target, real_target, PATH_MAX);
  if (fsutil_is_directory (real_source))
    {
    compat_printf ("Omitting directory '%s'\n", real_source);
    }
  else
    {
    if (strcasecmp (real_source, real_target) == 0)
      {
      compat_printf_stderr ("%s: '%s' and '%s' are the same file\n", argv0, 
        source, target); 
      }
    else
      {
      if (verbose) compat_printf ("%s -> %s\n", real_source, real_target);
      Error ret = fsutil_copy_file (real_source, real_target);
      if (ret == 0)
        {
        if (attributes)
          {
          int fd = sys_open (source, O_RDONLY);
          if (fd > 0)
            {
            struct stat sb;
            sys_fstat (fd, &sb);
            struct utimbuf times =  
              {
              .actime = sb.st_atime,
              .modtime = sb.st_mtime
              };
            sys_utime (target, &times);
            sys_close (fd);
            }
          }
        }
      else
        {
        compat_printf_stderr ("%s: %s\n", argv0, strerror (ret));
        }
      }
    }
  }

/*=========================================================================
  shell_cmd_cp_mv
=========================================================================*/
Error shell_cmd_cp_mv (int argc, char **argv, bool move)
  {
  int opt;
  optind = 0;
  Error ret = 0;
  BOOL usage = FALSE;
  BOOL verbose = FALSE;
  BOOL attributes = FALSE;
  while ((opt = getopt (argc, argv, "hvp")) != -1)
    {
    switch (opt)
      {
      case 'v':
        verbose = TRUE;
        break;
      case 'p':
        attributes = TRUE;
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
    if (argc - optind >= 2)
      {
      // cp a b c -- c must be a dir
      // cp a b -- a can be a file or dir. If dir, copy to b/a
      const char *raw_target = argv[argc - 1];

      BOOL multi = (argc - optind >= 3); // Multiple files to copy     

      BOOL target_is_dir = FALSE;
      if (fsutil_is_directory (raw_target))
        target_is_dir = TRUE;

      if (!target_is_dir && multi)
        {
        compat_printf_stderr 
           ("Last arg must be a dir, when copying/moving > 1 file\n");
        ret = EINVAL;
        }
      else
        {
        bool interrupted = false;
        for (int i = optind; (i < argc - 1) && (!interrupted); i++)
          {
          if (shell_poll_interrupt())
            {
            interrupted = true;
            }
          char real_target[PATH_MAX];
          const char *source = argv[i];
          if (target_is_dir)
            {
            // When we copy to a directory, we have to strip the dir part
            //   of the source. E.g., cp /bin/blink.lua /foo should 
            //   result in cp /bin/blink.lua /foo/blink.lua, not
            //   /foo/bin/blink.lua 
            char source_basename[PATH_MAX];
            fsutil_get_basename (source, source_basename, PATH_MAX);
            fsutil_join_path 
              (raw_target, source_basename, real_target, PATH_MAX);
            }
          else
            {
            strncpy (real_target, raw_target, PATH_MAX);
            }
          if (move)
            do_move_file (argv[0], source, real_target, verbose);
          else
            do_copy_file (argv[0], source, real_target, verbose, attributes);
          }
        if (interrupted)
          {
          shell_clear_interrupt();
          compat_printf_stderr ("%s: %s\n", argv[0], strerror (EINTR));
          ret = EINTR;
          }
        }
      }
    else
      {
      show_usage (argv[0]);
      ret = EINVAL;
      }
    }

  if (usage) ret = 0;
  return ret;
  }


