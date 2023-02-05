/*============================================================================
 *  shell/shell_cmd_mkdir.c
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
#include <shell/shell.h>
#include <klib/string.h>
#include <sys/syscalls.h>
#include <compat/compat.h>

/*=========================================================================
  shell_cmd_mkdir
=========================================================================*/
Error shell_cmd_mkdir (int argc, char **argv)
  {
  for (int i = 1; i < argc; i++)
    {
    Error err = sys_mkdir (argv[i]);
    if (err)
      {
      compat_printf_stderr ("%s: %s: %s\n", argv[0], argv[i], strerror (err));
      }
    }
  return 0;
  }






