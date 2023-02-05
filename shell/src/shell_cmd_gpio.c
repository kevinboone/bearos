/*============================================================================
 *  shell/shell_cmd_gpio.c
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

/*============================================================================
 * ==========================================================================*/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <pico/stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/error.h>
#include <errno.h>
#include <shell/shell.h>
#include <shell/shell_cmd.h>
#include <bearos/gpio.h>
#include <compat/compat.h>
#include <sys/syscalls.h>

/*=========================================================================
  show_usage 
=========================================================================*/
static void show_usage (const char *argv0)
  {
  compat_printf ("Usage: %s [options] {gpio_numbers ...}\n", argv0);
  compat_printf ("    -0     set pin low\n");
  compat_printf ("    -1     set pin high\n");
  compat_printf ("    -d     pull down\n");
  compat_printf ("    -i     set pin to input\n");
  compat_printf ("    -o     set pin to output\n");
  compat_printf ("    -p     prepare pin (reset, set to input)\n");
  compat_printf ("    -r     read pin\n");
  compat_printf ("    -u     pull up\n");
  compat_printf ("'-p' should be used to prepare the GPIO before any other\n");
  compat_printf ("operations. Default direction is input.\n");
  }

/*=========================================================================
  shell_cmd_gpio
=========================================================================*/
Error shell_cmd_gpio (int argc, char **argv)
  {
  int opt;
  optind = 0;
  Error ret = 0;
  BOOL usage = FALSE;
  char *cmd = "";
  while ((opt = getopt (argc, argv, "hprud10io")) != -1)
    {
    switch (opt)
      {
      case 'p': 
        cmd = GPIOCMD_INIT;
        break;
      case 'r': 
        cmd = GPIOCMD_READ;
        break;
      case 'u': 
        cmd = GPIOCMD_PULL_UP;
        break;
      case 'd': 
        cmd = GPIOCMD_PULL_DOWN;
        break;
      case '1': 
        cmd = GPIOCMD_SET_HIGH;
        break;
      case '0': 
        cmd = GPIOCMD_SET_LOW;
        break;
      case 'i': 
        cmd = GPIOCMD_SET_INPUT;
        break;
      case 'o': 
        cmd = GPIOCMD_SET_OUTPUT;
        break;
      case 'h':
        usage = TRUE;
        // Fall through
      default:
        show_usage (argv[0]);
        ret = EINVAL;
      }
    }

  if (ret == 0 && !cmd[0])
    {
    compat_printf_stderr ("%s: No GPIO operation specified\n", argv[0]);
    ret = EINVAL;
    }

  if (ret == 0)
    {
    if (argc - optind >= 1)
      {
      int fd = sys_open ("p:/gpio", O_RDWR);
      char buff[10];
      for (int i = optind; i < argc; i++)
	{
        int pin = atoi (argv[i]);
        sprintf (buff, "%s%02d", cmd, pin);
        sys_write (fd, buff, 5);

        if (strcmp (cmd, GPIOCMD_READ) == 0)
          {
          sys_read (fd, buff, 2);
          compat_printf ("%c\n", buff[0]);
          }
        }
      sys_close (fd);
      }
    else
      {
      compat_printf_stderr ("%s: No GPIO pins specified\n", argv[0]);
      ret = EINVAL;
      }
    }

  if (usage) ret = 0;
  return ret;
  return 0;
  }

