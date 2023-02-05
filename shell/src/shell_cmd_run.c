/*============================================================================
 *  shell/shell_cmd_run.c
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
#include <sys/elf.h>
#include <sys/syscalls.h>
#include <sys/syscall.h>
#include <compat/compat.h>
#include <bearos/exec.h>


/*=========================================================================
  shell_cmd_run_file
=========================================================================*/
Error shell_cmd_run_file (const char *path, int argc, char **argv)
  {
  Process *current = process_get_current();

  Process *p = process_new_clone (current);
 
  // This new process has no open files at this point
  for (int i = 0; i < NFILES; i++) p->files[i] = current->files[i]; 
  Error ret = process_run_file (p, path, argc, argv);

  for (int i = 0; i < NFILES; i++) p->files[i] = NULL; 

  process_destroy (p);
  return ret;
  }

