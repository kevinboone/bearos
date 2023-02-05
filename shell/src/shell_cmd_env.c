/*============================================================================
 *  shell/shell_cmd_env.c
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

/*=========================================================================
  show_usage 
=========================================================================*/
static void show_usage (const char *argv0)
  {
  compat_printf ("Usage: %s [name=value...]\n", argv0);
  compat_printf ("Print or set environment values.\n", argv0);
  }

/*=========================================================================
  dump_env 
=========================================================================*/
static void dump_env (void)
  {
  Process *p = process_get_current();
  char **envp = process_get_envp (p);
  while (*envp)
    {
    compat_printf ("%s\n", *envp);
    envp++;
    }
  }

/*=========================================================================
  set_env 
=========================================================================*/
static Error set_env (const char *env_tok, const char *argv0)
  {
  Error ret = 0;
  char *s_ = strdup (env_tok);
  char *eqpos = strchr (s_, '='); 
  char *name = s_; 
  if (!eqpos)
    {
    compat_printf_stderr ("%s: %s: %s\n", argv0, env_tok, "No equal sign");
    ret = EINVAL;
    }
  else 
    {
    *eqpos = 0;
    if (name[0] == 0)
      {
      compat_printf_stderr ("%s: %s: %s\n", argv0, env_tok, "No name");
      ret = EINVAL;
      }
    else
      shell_add_variable (env_tok);
    }
  free (s_);
  return ret;
  }

/*=========================================================================
  shell_cmd_env
=========================================================================*/
Error shell_cmd_env (int argc, char **argv)
  {
  Error ret = 0;
  int opt;
  optind = 0;
  BOOL usage = FALSE;

  while ((opt = getopt (argc, argv, "h")) != -1)
    {
    switch (opt)
      {
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
      dump_env();
      }
    else
      {
      for (int i = optind; i < argc; i++)
        {
        ret = set_env (argv[i], argv[0]);
        }
      }
    }

  if (usage) ret = 0;
  return ret;
  }


