/*============================================================================
 *  shell/shell_cmd_mv.c
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
#include <getopt.h>
#include <sys/error.h>
#include <shell/shell.h>
#include <shell/shell_cmd.h>

/*=========================================================================
  shell_cmd_mv
=========================================================================*/
Error shell_cmd_mv (int argc, char **argv)
  {
  return shell_cmd_cp_mv (argc, argv, true);
  }









