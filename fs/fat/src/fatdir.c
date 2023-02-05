/*============================================================================
 *  fsmanager/fatdir.c
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

/*============================================================================
 * ==========================================================================*/

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/error.h>
#include <errno.h>
#include <sys/direntry.h>
#include <sys/filedesc.h>
#include <fat/fat.h>
#include <fat/fatdir.h>
#include <ff.h>

struct _FATDir
  {
  FileDesc *filedesc;
  DIR dp;
  };

/*============================================================================
 * fatdir_close
 * ==========================================================================*/
Error fatdir_close (FileDesc *descriptor)
  {
  FATDir *self = descriptor->self;
  f_closedir (&(self->dp));
  fatdir_destroy (self);
  return 0;
  }

/*============================================================================
 * fatdir_open
 * ==========================================================================*/
Error fatdir_open (FileDesc *descriptor, int flags)
  {
  (void)flags;
  FATDir *self = descriptor->self;
  // FATDir *self = descriptor->self;
  descriptor->type = S_IFDIR;
  FRESULT fr = f_opendir (&(self->dp), descriptor->name); 
  return fat_fresult_to_error (fr); 
  }

/*============================================================================
 * fatdir_write
 * ==========================================================================*/
int fatdir_write (FileDesc *descriptor, const void *buffer, int len)
  {
  (void)descriptor; (void)buffer; (void)len;
  return -ENOSYS;
  }

/*============================================================================
 * fatdir_read
 * ==========================================================================*/
int fatdir_read (FileDesc *descriptor, void *buffer, int len)
  {
  FATDir *self = descriptor->self;
  DirEntry *de = buffer;
  FILINFO finfo;
  FRESULT fr = f_readdir (&(self->dp), &finfo);
  if (fr == 0)
    {
    if (finfo.fname[0] == 0)
      return 0;
    else
      {
      strncpy (de->name, finfo.fname, PATH_MAX);
      if (finfo.fattrib & AM_DIR)
        de->type = S_IFDIR;
      else
        de->type = S_IFREG;
      de->size = (int32_t)finfo.fsize;
      de->mtime = fat_time_to_unix (finfo.ftime | finfo.fdate << 16);
      return len;
      }
    }
  else
    {
    return -fat_fresult_to_error (fr);
    }
  return 0;
  }

/*============================================================================
 * fatdir_read_timeout
 * ==========================================================================*/
int fatdir_read_timeout (FileDesc *descriptor, int msec)
  {
  (void)descriptor; (void)msec;
  return -ENOSYS;
  }

/*============================================================================
 * fatdir_get_size
 * ==========================================================================*/
int32_t fatdir_get_size (FileDesc *descriptor)
  {
  (void)descriptor; 
  return 0;
  }

/*============================================================================
 * fatdir_new
 * ==========================================================================*/
FATDir *fatdir_new (const char *path)
  {
  FATDir *self = malloc (sizeof (FATDir));
  FileDesc *filedesc = filedesc_new ();
  filedesc->name = strdup (path);
  filedesc->close = fatdir_close;
  filedesc->open = fatdir_open;
  filedesc->read = fatdir_read;
  filedesc->write = fatdir_write;
  filedesc->get_size = fatdir_get_size;
  filedesc->read_timeout = fatdir_read_timeout;
  filedesc->self = self;
  self->filedesc = filedesc;
  return self;
  }

/*============================================================================
 * fatdir_destroy
 * ==========================================================================*/
void fatdir_destroy (FATDir *self)
  {
  free (self->filedesc->name);
  free (self->filedesc);
  free (self);
  }

/*============================================================================
 * fatdir_get_filedesc
 * ==========================================================================*/
FileDesc *fatdir_get_filedesc (FATDir *self)
  {
  return self->filedesc;
  }


