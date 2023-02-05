/*============================================================================
 *  shell/shell_cmd_grep.c
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
//#include <regex.h>
#include <compat/re.h>
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

/*=========================================================================
  strlwr
=========================================================================*/
char *strlwr (char *s)
  {
  char *ss = s;
  while (*ss)
    {
    *ss = (char)tolower (*ss);
    ss++;
    }
  return s;
  }

/*=========================================================================
  show_usage 
=========================================================================*/
static void show_usage (const char *argv0)
  {
  compat_printf ("Usage: %s {pattern} {files...}\n", argv0);
  compat_printf ("Searches for patterns in files.\n", argv0);
  compat_printf ("  -c: show match count\n", argv0);
  compat_printf ("  -i: case-insensitive\n", argv0);
  compat_printf ("  -n: don't show filenames with multiple files\n", argv0);
  compat_printf ("  -v: show non-matches\n", argv0);
  compat_printf ("If no files are listed, searches stdin.\n", argv0);
  }

/*=========================================================================
  do_grep_line
  Returns true if a line matches
=========================================================================*/
static BOOL do_grep_line (const char *filename, struct regex_t *preg, 
        const char *line, BOOL invert, BOOL show_filename, 
        BOOL suppress, BOOL nocase)
  {
  BOOL show = invert ? TRUE : FALSE; 
  BOOL match = FALSE;
  int len = 0;
  if (nocase)
    {
    char *testline = strdup (line);
    strlwr (testline);
    if (re_matchp (preg, testline, &len) != -1)
      show = invert ? FALSE : TRUE; 
    free (testline);
    }
  else
    {
    if (re_matchp (preg, line, &len) != -1)
      show = invert ? FALSE : TRUE; 
    }

  if (show)
    {
    match = TRUE;
    if (!suppress)
      {
      if (show_filename)
	compat_printf ("%s: %s\n", filename, line);
      else
	compat_printf ("%s\n", line);
      }
    }
  return match;
  }

/*=========================================================================
  do_grep_fd
  Returns the number of matching lines. There is no error return -- we 
    assume that if the fd has been opened, it can likely be read.
=========================================================================*/
static int do_grep_fd (const char *filename, struct regex_t *preg, int fd, 
        BOOL invert, BOOL show_filename, BOOL show_count, BOOL nocase)
  {
  int curr_len = 0;
  int max_len = 80;
  int expand = 80;
  int count = 0;

  char *line = malloc ((size_t)max_len);
  char buff[40];
  line[0] = 0;
  curr_len = 0;
  int n = sys_read (fd, buff, sizeof (buff));
  while (n > 0)
    {
    if (n + curr_len > max_len)
      {
      max_len += expand;
      line = realloc (line, (size_t)max_len); 
      }
    strncat (line, buff, (size_t)n);       
    curr_len += n;
    line[curr_len] = 0; 

    char *nlpos;
    while ((nlpos = strchr (line, '\n')))
      {
      int this_len = (int)(nlpos - line);
      line[this_len] = 0;
      if (do_grep_line (filename, preg, line, invert, 
          show_filename, show_count, nocase)) count++;
      memmove (line, line + this_len + 1, (size_t)(curr_len - this_len)); 
      curr_len -= this_len;
      }

    n = sys_read (fd, buff, sizeof (buff));
    }
  
  if (curr_len > 0 && line[0] && line[0] != '\n')
    {
    if (do_grep_line (filename, preg, line, invert, show_filename, 
      show_count, nocase))
       count++;
    }

  free (line);

  if (show_count)
    {
    if (show_filename)
      compat_printf ("%s: %d\n", filename, count);
    else
      compat_printf ("%d\n", count);
    }

  return count;
  }

/*=========================================================================
  do_grep_file 
=========================================================================*/
static Error do_grep_file (struct regex_t *preg, 
               const char *filename, const char *argv0, BOOL invert, 
               BOOL show_filename, BOOL show_count, BOOL nocase)
  {
  Error ret = 0;
  int fd = sys_open (filename, O_RDONLY); // TODO DIR
  if (fd >= 0)
    {
    do_grep_fd (filename, preg, fd, invert, 
         show_filename, show_count, nocase); 
    sys_close (fd);
    }
  else
    {
    ret = -fd;
    compat_printf_stderr ("%s: %s: %s\n", argv0, filename, strerror (ret));
    }
  return ret;
  }

/*=========================================================================
  shell_cmd_grep
=========================================================================*/
Error shell_cmd_grep (int argc, char **argv)
  {
  Error ret = 0;
  int opt;
  optind = 0;
  BOOL nocase = FALSE;
  BOOL usage = FALSE;
  BOOL no_filename = FALSE;
  BOOL invert = FALSE;
  BOOL count = FALSE;

  while ((opt = getopt (argc, argv, "hinvc")) != -1)
    {
    switch (opt)
      {
      case 'c':
        count = TRUE;
        break;
      case 'i':
        nocase = TRUE;
        break;
      case 'n':
        no_filename = TRUE;
        break;
      case 'v':
        invert = TRUE;
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
      show_usage (argv[0]); 
      ret = EINVAL;
      }
    else
     {
     struct regex_t *preg;
     char *pattern = strdup (argv[optind]);
     if (nocase) strlwr (pattern);
     if ((preg = re_compile (argv[optind])) != 0)
       {
       if (argc - optind == 1)
         {
         ret = do_grep_fd ("stdin", preg, 0, invert, FALSE, count, nocase);
         }
       else
         {
         BOOL show_filename = FALSE;
         for (int i = optind + 1; i < argc && ret == 0; i++)
            {
            if (argc - optind == 2)
              show_filename = FALSE;
            else
              show_filename = !no_filename;
 
            ret = do_grep_file (preg, argv[i], argv[0], 
                   invert, show_filename, count, nocase);
            }
          }
        //regfree (&preg);
        }
      else
        {
        compat_printf_stderr ("%s: bad expression: %s\n", argv[0], pattern);
        ret = EINVAL;
        }
      free (pattern);
      }
    }
  if (usage) ret = 0;
  return ret;
  }


