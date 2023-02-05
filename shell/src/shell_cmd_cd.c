/*============================================================================
 *  shell/shell_cmd_cd.c
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
#include <term/term.h>
#include <shell/shell.h>
#include <klib/string.h>
#include <sys/syscalls.h>
#include <compat/compat.h>


/*=========================================================================
  shell_cmd_cd
=========================================================================*/
Error shell_cmd_cd (int argc, char **argv)
  {
  if (argc == 1)
    {
    char buff[PATH_MAX];
    sys_getcwd (buff, PATH_MAX);
    compat_printf ("%s\n", buff);
    return 0;
    }
  else if (argc == 2)
    {
    Error err = sys_chdir (argv[1]);
    if (err)
      {
      compat_printf_stderr ("%s: %s: %s\n", argv[0], argv[1], strerror (err));
      return err;
      }
    else
      return 0;
    }
  else
    {
    compat_printf_stderr ("Usage: cd [path]\n");
    return EINVAL;
    }
  }



