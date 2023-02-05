/*============================================================================
 *  shell/shell_cmd_ls.c
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
#include <bearos/devctl.h>

/*=========================================================================
  write_type_char 
=========================================================================*/
static char get_type_char (uint32_t mode)
  {
  char c;
  switch (mode & S_IFMT)
    {
    case S_IFBLK: c = 'b'; break;
    case S_IFCHR: c = 'c'; break;
    case S_IFDIR: c = 'd'; break;
    case S_IFIFO: c = 'p'; break;
    case S_IFLNK: c = 'l'; break;
    case S_IFSOCK: c = 's'; break;
    default: c = '-';
    }
  return c;
  }

/*=========================================================================
  convert_date 
=========================================================================*/
static void convert_date (time_t datetime, char *s, int len)
  {
  struct tm mytm;
  sys_gmtime_r (datetime, &mytm);
  
  snprintf (s, (size_t)len, "%04d/%02d/%02d %02d:%02d", 
    mytm.tm_year + 1900, 
    mytm.tm_mon + 1,
    mytm.tm_mday,
    mytm.tm_hour,
    mytm.tm_min);
  }

/*=========================================================================
  do_path 
=========================================================================*/
static void do_path (const char *argv0, const char *path, BOOL fmt_long)
  {
  char abspath[PATH_MAX];
  fsutil_make_abs_path (path, abspath, PATH_MAX);
  int max_across = 1;
  int fd = sys_open (abspath, O_RDONLY);
  uint32_t flags;
  int width = 80;

  BOOL isatty = FALSE; 
  if (sys_devctl (1, DC_GET_GEN_FLAGS, (intptr_t)&flags) == 0)
    {
    if (flags & DC_FLAG_ISTTY)
      isatty = TRUE;
    }
  if (isatty)
    {
    DevCtlTermProps props;
    if (sys_devctl (1, DC_TERM_GET_PROPS, (intptr_t)&props) == 0)
      {
      width = props.cols;
      }
    }

  if (fd >= 0)
    {
    struct stat sb;
    sys_fstat (fd, &sb);
    if ((sb.st_mode & S_IFMT) == S_IFDIR)
      {
      int fd2;
      DirEntry de;
      int max_namelen = 1;
      if (!fmt_long && isatty)
        {
        fd2 = sys_open (abspath, O_RDONLY); // Should succeed -- already open
        while (sys_getdent (fd2, &de) > 0)
          {
          int namelen = (int)strlen (de.name); 
          if (namelen > max_namelen) max_namelen = namelen;
          }
        sys_close (fd2);
        max_namelen += 1; // Allow at least one space between names
        max_across = width / max_namelen; 
        }
      else
        max_across = 1;
      fd2 = sys_open (abspath, O_RDONLY); // Should succeed -- already open
      int across = 0;
      while (sys_getdent (fd2, &de) > 0)
        {
        if (fmt_long)
          {
          char s[30];
          convert_date (de.mtime, s, sizeof (s));
          compat_printf ("%c %8ld %s %s\n", get_type_char ((uint32_t)de.type), 
            de.size, s, de.name); 
          }
        else
          {
          compat_printf ("%s", de.name); 
          int pad = max_namelen - (int)strlen (de.name);
          for (int j = 0; j < pad; j++) compat_printf (" ");
          across++;
          if (across >= max_across)
            {
            compat_printf ("\n");
            across = 0;
            }
          }
        }
      if (across != 0) compat_printf ("\n");
      sys_close (fd2);
      }
    else
      {
      char s[30];
      convert_date (sb.st_mtime, s, sizeof (s));
      compat_printf ("%c %8ld %s %s\n", get_type_char ((uint32_t)sb.st_mode), 
          sb.st_size, s, path);
      } 
    sys_close (fd);
    }
  else
    {
    Error ret = -fd;
    compat_printf_stderr ("%s: %s: %s\n", argv0, abspath, strerror (ret));
    }
  }

/*=========================================================================
  show_usage 
=========================================================================*/
static void show_usage (const char *argv0)
  {
  compat_printf ("Usage: %s [-l] [drive, path, file]\n", argv0);
  compat_printf ("             -l    long listing\n", argv0);
  compat_printf ("List files or directories\n", argv0);
  }

/*=========================================================================
  shell_cmd_ls
=========================================================================*/
Error shell_cmd_ls (int argc, char **argv)
  {
  Error ret = 0;
  const char *path;
  int opt;
  optind = 0;
  BOOL usage = FALSE;
  BOOL fmt_long = FALSE;

  while ((opt = getopt (argc, argv, "hl")) != -1)
    {
    switch (opt)
      {
      case 'l':
        fmt_long = TRUE;
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
    if (argc - optind == 0)
      {
      Process *p = process_get_current();
      path = process_get_cwd (p);
      do_path (argv[0], path, fmt_long);
      }
    else
      {
      for (int i = optind; i < argc; i++)
        {
        path = argv[i];
        compat_printf ("%s:\n", path);
        do_path (argv[0], path, fmt_long);
        }
      }
    }

  if (usage) ret = 0;
  return ret;
  }


