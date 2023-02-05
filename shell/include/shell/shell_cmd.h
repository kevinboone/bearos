/*============================================================================
 *  shell/shell_cmd.h
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

typedef Error (*ShellCmdFn) (int argc, char **argv);

#ifdef __cplusplus
extern "C" {
#endif

extern Error shell_cmd_df (int argc, char **argv);
extern Error shell_cmd_ls (int argc, char **argv);
extern Error shell_cmd_cd (int argc, char **argv);
extern Error shell_cmd_cat (int argc, char **argv);
extern Error shell_cmd_date (int argc, char **argv);
extern Error shell_cmd_echo (int argc, char **argv);
extern Error shell_cmd_env (int argc, char **argv);
extern Error shell_cmd_gpio (int argc, char **argv);
extern Error shell_cmd_grep (int argc, char **argv);
extern Error shell_cmd_mkdir (int argc, char **argv);
extern Error shell_cmd_cp (int argc, char **argv);
extern Error shell_cmd_clear (int argc, char **argv);
extern Error shell_cmd_mv (int argc, char **argv);
extern Error shell_cmd_rm (int argc, char **argv);
extern Error shell_cmd_rmdir (int argc, char **argv);
extern Error shell_cmd_run_file (const char *path, int argc, char **argv);
extern Error shell_cmd_source (int argc, char **argv);
extern Error shell_cmd_uname (int argc, char **argv);

extern Error shell_cmd_cp_mv (int argc, char **argv, bool move);

Error shell_cmd (int argc, char **argv);

#ifdef __cplusplus
}
#endif



