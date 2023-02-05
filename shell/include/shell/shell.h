/*============================================================================
 *  shell/shell.h
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

#pragma once

/*============================================================================
 * ==========================================================================*/

#include <stdint.h>
#include <sys/error.h>
#include <klib/list.h>
#include <sys/process.h>
#include <sys/error.h>

#define SHELL_MAX_LINE 128

#ifdef __cplusplus
extern "C" {
#endif

extern bool shell_poll_interrupt (void);
extern void shell_clear_interrupt (void);

extern Error shell_add_variable (const char *env_tok);

extern void shell_run ();
Error shell_do_line (const char *buff, int argc, char **argv);

extern Error shell_process (int argc, char **argv,
                             char **envp);

#ifdef __cplusplus
}
#endif


