/*============================================================================
 *  fsmanager/fsutil.h
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

#pragma once

/*============================================================================
 * ==========================================================================*/

#include <stdint.h>
#include <pico/stdlib.h>
#include <sys/error.h>

#ifdef __cplusplus
extern "C" {
#endif

/* *Get the filename part of a path, after the last /, or the whole thing
   if there is no / */
extern void fsutil_get_basename (const char *path, char *result, int16_t len);

/** Get the directory part of a path. */
extern void fsutil_get_dir (const char *path, char *result, int16_t len);

void fsutil_join_path (const char *dir, const char *basename, 
      char *newpath, int16_t len);

bool fsutil_is_directory (const char *path);

bool fsutil_is_regular (const char *path);
  
/** Convert a path, which might be relative to the current directory.
      to a full path including a drive letter. See docs/paths.md
      for more information. */
char *fsutil_make_abs_path (const char *in, char *out, int len);

Error fsutil_copy_file (const char *source, const char *real_target);

#ifdef __cplusplus
}
#endif

