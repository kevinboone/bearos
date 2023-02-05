/*============================================================================
 *  sys/process.h
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

#pragma once

#include <stdint.h>
#include <sys/limits.h>
#include <sys/filedesc.h>
#include <sys/error.h>
#include <sys/environment.h>
#include <setjmp.h>

struct _Process; 

typedef Error (*EntryFn)(int argc, char **argv, char **envp);

typedef struct _Process
  {
  char name[PROCESS_MAX_NAME];
  char cwd[PATH_MAX];
  FileDesc *files[NFILES];
  Environment *environment;
  void *brk;
  char reserved[32];
  jmp_buf exit_jmp;
  } Process;

#ifdef __cplusplus
extern "C" {
#endif

extern Process *process_new (void);
extern Process *process_new_clone (const Process *old);

extern void process_destroy (Process *self);
extern Error process_run (Process *self, EntryFn fn, int argc,
                         char **argv);
extern Error process_run_file (Process *self, const char *path, int argc,
                         char **argv);

extern Error process_set_filedesc (Process *self, int n, FileDesc *filedesc);
extern Error process_get_filedesc (Process *self, int n, FileDesc **filedesc);
extern Error process_release_filedesc (Process *self, int n);

/** Sets the current process, and returns the previous one. */
extern Process *process_set_current (Process *p);
extern Process *process_get_current (void);

extern Error process_get_next_fd (const Process *self, int *fd);
extern int8_t process_get_current_drive (const Process *self);
extern const char *process_get_cwd (const Process *self);
extern Error process_set_cwd (Process *self, const char *cwd);
extern int process_get_utc_offset (const Process *self);

extern const char *process_getenv (const Process *self, const char *name);
extern void process_setenv (Process *self, const char *name, const char *value);

extern void process_set_break (Process *self, void *brk);
extern void *process_get_break (const Process *self);

extern char **process_get_envp (Process *self);

extern Error process_open_file (Process *self, int fd, 
         const char *path, int flags);

#ifdef __cplusplus
}
#endif




