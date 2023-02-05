/*============================================================================
 *  fsmanager/fsutil.c
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/
#define _GNU_SOURCE

/*============================================================================
 * ==========================================================================*/

#include <stdint.h>
#include <pico/stdlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/error.h>
#include <errno.h>
#include <sys/syscalls.h>
#include <sys/process.h>
#include <bearos/intr.h>

/*============================================================================
 * fsutil_get_basename 
 * ==========================================================================*/
void fsutil_get_basename (const char *path, char *result, int16_t len) 
  {
  char *p = strrchr (path, '/');
  if (p)
    {
    strncpy (result, p + 1, (size_t)len);
    }
  else
    {
    strncpy (result, path, (size_t)len);
    }
  }

/*============================================================================
 * fsutil_get_dir
 * ==========================================================================*/
void fsutil_get_dir (const char *path, char *result, int16_t len) 
  {
  if (strlen (path) >= 2 && path[1] == ':')
    {
    fsutil_get_dir (path + 2, result + 2, len - 2);
    result[0] = path[0];
    result[1] = path[1];
    return;
    }

  strncpy (result, path, (size_t)len);
  if (strcmp (result, "/") == 0) return; // Handle "/" case separately
  char *p = strrchr (result, '/');
  if (p)
    {
    *(p + 1) = 0;
    }
  else
    {
    *result = 0;
    return;
    }
  int l = (int)strlen (result);
  while (l > 1 && result[l - 1] == '/')
    {
    // Strip the last /, unless that would leave the result bare
    result[l - 1] = 0;
    l--;
    }
  }

/*============================================================================
 * fsutil_join_path
 * ==========================================================================*/
void fsutil_join_path (const char *dir, const char *basename, 
      char *result, int16_t len)
  {
  if (dir[0])
    {
    strncpy (result, dir, (size_t)len);
    }
  else
    result[0] = 0;

  if (result[0] != 0 && dir[strlen(dir) - 1] != '/')
    strcat (result, "/");

  strncat (result, basename, (size_t)len - strlen (result));
  }

/*============================================================================
 * fsutil_is_directory
 * ==========================================================================*/
bool fsutil_is_directory (const char *path)
  {
  bool ret = false;
  int fd = sys_open (path, O_RDONLY);
  if (fd >= 0)
    {
    struct stat sb;
    if (sys_fstat (fd, &sb) == 0)
      {
      ret = ((sb.st_mode & S_IFMT) == S_IFDIR);
      }
    sys_close (fd);
    }
  return ret;
  }

/*============================================================================
 * fsutil_is_regular
 * ==========================================================================*/
bool fsutil_is_regular (const char *path)
  {
  bool ret = false;
  int fd = sys_open (path, O_RDONLY);
  if (fd >= 0)
    {
    struct stat sb;
    if (sys_fstat (fd, &sb) == 0)
      {
      ret = ((sb.st_mode & S_IFMT) == S_IFREG);
      }
    sys_close (fd);
    }
  return ret;
  }

/*============================================================================
 * fsutil_make_abs_path
 * ==========================================================================*/
static char *__realpath (const char *name, char *resolved, int len)
  {
  char *rpath, *dest;
  const char *start, *end;
  int lost_drive = -1;

  len --; // Leave room for the \0

  if (name == NULL)
    {
    return NULL;
    }

  if (name[0] == '\0')
    {
    return NULL;
    }

  if (name[1] == ':')
    {
    resolved[0] = name[0];
    resolved[1] = name[1];
    return __realpath (name + 2, resolved + 2, len - 2);
    }

  rpath = resolved;

  if (name[0] != '/')
    {
    sys_getcwd (rpath, PATH_MAX); 
    lost_drive = rpath[0];
    memmove (rpath, rpath + 2, PATH_MAX);
    dest = rawmemchr (rpath, '\0');
    }
  else
    {
    rpath[0] = '/';
    dest = rpath + 1;
    }

  for (start = end = name; *start; start = end)
    {
    while (*start == '/')
      ++start;

    for (end = start; *end && *end != '/'; ++end);

    if (end - start == 0)
      break;
    else if (end - start == 1 && start[0] == '.');
    else if (end - start == 2 && start[0] == '.' && start[1] == '.')
      {
      if (dest > rpath + 1) 
        {
        while ((--dest)[-1] != '/');
        }
      }
    else
      {
      if (dest[-1] != '/')
         *dest++ = '/';

      dest = mempcpy (dest, start, (size_t)(end - start));
      *dest = '\0';
      }
    }

  if (dest > rpath + 1 && dest[-1] == '/')
    --dest;
  *dest = '\0';

  if (lost_drive >= 0)
      {
      memmove (resolved + 2, resolved, (size_t)(len - 2));
      resolved[0] = (char)lost_drive;
      resolved[1] = ':';
      }

  return rpath;
  }

char *fsutil_make_abs_path (const char *in, char *out, int len) 
  {
  int drive_letter = -1;
  char temp[PATH_MAX];

  if (!in[0])
    {
    // Blank path -- what can we do?
    out[0] = 0;
    return NULL;
    }

  if (in[1] == ':')
    {
    // We start with a drive letter
    drive_letter = in[0];
    if (in[2] == '/')
      strcpy (temp, in + 2);
    else
      {
      strcpy (temp + 1, in + 2);  
      temp[0] = '/';
      }
    }
  else
    {
    strcpy (temp, in);
    }

  if (drive_letter >= 0)
    {
    __realpath (temp, out + 2, len - 2);
    out[0] = (char)drive_letter;
    out[1] = ':';
    }
  else
    {
    if (temp[0] == '/')
      {
      __realpath (temp, out + 2, len - 2);
      out[0] = (char)(process_get_current_drive (process_get_current()) + 'A'); 
      out[1] = ':';
      }
    else
      __realpath (temp, out, len);
    }
 
  // Don't leave a bare colon at the end of a path
  if (out[1] == ':' && out[2] == '\0')
    {
    out[2] = '/'; 
    out[3] = '\0';
    }
  return out;
  }

/*=========================================================================
  fsutil_copy_file
=========================================================================*/
Error fsutil_copy_file (const char *source, const char *target)
  {
  Error ret = 0;
  char buff[512];
  bool interrupted = false;
  int in = sys_open (source, O_RDONLY);
  if (in >= 0)
    {
    int out = sys_open (target, O_WRONLY | O_TRUNC);
    if (out >= 0)
      {
      int n;
      do
        {
        interrupted = sys_poll_interrupt (SYS_INTR_TERM);
        n = sys_read (in, buff, sizeof (buff));
        if (n > 0)
          sys_write (out, buff, n);
        if (n <= 0) ret = -n;
        } while (n > 0 && !interrupted);
      sys_close (out);
      }
    else
      ret = -out;
    sys_close (in);
    }
  else
    ret = -in;

  if (interrupted)
    {
    sys_clear_interrupt (SYS_INTR_TERM);
    }
  return ret;
  }


