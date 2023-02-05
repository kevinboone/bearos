/*============================================================================
 *  sys/environment.h
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

#pragma once

#include <sys/error.h>
#include <pico/stdlib.h>

struct _Environment;
typedef struct _Environment Environment;

#ifdef __cplusplus
extern "C" {
#endif

extern Environment *environment_new (void);
extern Environment *environment_clone (const Environment *old);

extern void environment_destroy (Environment *self);

extern const char *environment_get (const Environment *self, const char *name);

/** Add an entry to the environment, without deleting any existing entry. This
     is faster than _set but, of course, will break if the entry already 
     exists. */
extern Error environment_add (Environment *self, 
         const char *name, const char *value);

/** Add an entry that is already formatted as 'name=value'. */
extern Error environment_add_raw (Environment *self, const char *token);

extern Error environment_set (Environment *self, const char *name, 
         const char *value, bool overwrite);

extern void environment_delete (Environment *self, const char *name);

extern char **environment_get_envp (const Environment *self);

extern int environment_size (const Environment *self);

#ifdef __cplusplus
}
#endif

