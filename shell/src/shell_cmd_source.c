/*============================================================================
 *  shell/shell_cmd_source.c
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
#include <shell/shell.h>
#include <klib/string.h>
#include <sys/syscalls.h>
#include <compat/compat.h>

/*=========================================================================
  shell_cmd_source
=========================================================================*/
static Error run_script (const char *filename, int argc, char **argv)
  {
  int ret = 0;
  int curr_len = 0;
  int max_len = 20;
  int expand = 10;
  char *line = malloc ((size_t)max_len);
  int fd = sys_open (filename, O_RDONLY);
  if (fd)
    {
    int new_argc = argc;
    char **new_argv = argv;

    /*
    int new_argc = argc - 1;
    char **new_argv = malloc ((size_t)(new_argc + 1) * sizeof (char *));
    for (int i = 0; i < new_argc; i++)
      new_argv[i] = strdup (argv[i + 1]);
    new_argv[new_argc] = 0;
    */

    char buff[8]; // TODO
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
       while ((nlpos = strchr (line, '\n')) && ret == 0)
         {
         int this_len = (int)(nlpos - line);
         line[this_len] = 0;
         ret = shell_do_line (line, new_argc, new_argv);
         memmove (line, line + this_len + 1, (size_t)(curr_len - this_len)); 
         curr_len -= this_len;
         }

       n = sys_read (fd, buff, sizeof (buff));
       }
  
    if (curr_len > 0 && line[0] && line[0] != '\n')
      ret = shell_do_line (line, new_argc, new_argv);

    free (line);
    sys_close (fd);

    /*
    for (int i = 0; i < new_argc; i++)
      free (new_argv[i]);
    free (new_argv);
    */
    } 
  else
    {
    compat_printf_stderr ("%s: %s: %s\n", argv[0], filename, 
       strerror (errno));
    ret = errno;
    }
  return ret;
  }

/*=========================================================================
  shell_cmd_source
=========================================================================*/
Error shell_cmd_source (int argc, char **argv)
  {
  if (argc <= 1)
    {
    compat_printf ("Usage: %s {script} [args]\n", argv[0]);
    return EINVAL;
    }

  int new_argc = argc - 1;
  char **new_argv = malloc ((size_t)new_argc * sizeof (char *));
  for (int i = 0; i < new_argc; i++)
    new_argv[i] = strdup (argv[i + 1]);

  int ret = run_script (argv[1], new_argc, new_argv);

  for (int i = 0; i < new_argc; i++)
    free (new_argv[i]);
 
  free (new_argv);

  return ret;
  }







