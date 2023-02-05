/*============================================================================
 *  fsmanager/fatfile.c
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
#include <fat/fatfile.h>
#include <ff.h>

struct _FATFile
  {
  FileDesc *filedesc;
  int flags;
  FIL fp;
  };

/*============================================================================
 * fatfile_close
 * ==========================================================================*/
Error fatfile_close (FileDesc *descriptor)
  {
  (void)descriptor;
  FATFile *self = descriptor->self;
  f_close (&(self->fp));
  fatfile_destroy (self);
  return 0;
  }

/*============================================================================
 * fatfile_open
 * ==========================================================================*/
Error fatfile_open (FileDesc *descriptor, int flags)
  {
  (void)flags;
  FATFile *self = descriptor->self;
  uint8_t mode = fat_open_to_f_open_mode (flags);
  descriptor->type = S_IFREG;
  FRESULT fr = f_open (&(self->fp), descriptor->name, mode);
  return fat_fresult_to_error (fr);
  }

/*============================================================================
 * fatfile_write
 * ==========================================================================*/
int fatfile_write (FileDesc *descriptor, const void *buffer, int len)
  {
  (void)descriptor; (void)buffer; (void)len;
  FATFile *self = descriptor->self;
  int count;
  FRESULT fr = f_write (&(self->fp), buffer, (UINT)len, (UINT *)&count);	
  if (fr == 0) 
    return count;
  else
    return -fat_fresult_to_error (fr);
  }

/*============================================================================
 * fatfile_read
 * ==========================================================================*/
int fatfile_read (FileDesc *descriptor, void *buffer, int len)
  {
  FATFile *self = descriptor->self;
  int count;
  FRESULT fr = f_read (&(self->fp), buffer, (UINT)len, (UINT *)&count);	
  if (fr == 0) 
    return count;
  else
    return -fat_fresult_to_error (fr);
  }

/*============================================================================
 * fatfile_read_timeout
 * ==========================================================================*/
int fatfile_read_timeout (FileDesc *descriptor, int msec)
  {
  (void)descriptor; (void)msec;
  return -ENOSYS;
  }

/*============================================================================
 * fatfile_get_size
 * ==========================================================================*/
int32_t fatfile_get_size (FileDesc *descriptor)
  {
  const char *path = descriptor->name;
  FILINFO fno;
  f_stat (path, &fno);
  return (int32_t)fno.fsize;
  }

/*============================================================================
 * fatfile_lseek
 * ==========================================================================*/
int32_t fatfile_lseek (FileDesc *desc, int32_t offset, int origin)
  {
  FATFile *self = desc->self;
  int32_t pos = (int32_t)self->fp.fptr;
  if (origin == SEEK_CUR)
     pos += offset;
  else if (origin == SEEK_END)
    {
    FILINFO fno;
    f_stat (desc->name, &fno);
    pos = (int32_t)(fno.fsize - (FSIZE_t)offset);
    }
  else
    {
    pos = offset;
    }

  FRESULT fr = f_lseek (&(self->fp), (FSIZE_t)pos);
  if (fr == 0)
     return offset;
  else
     return -fat_fresult_to_error (fr);
  }

/*============================================================================
 * fatfile_truncate
 * ==========================================================================*/
Error fatfile_truncate (FileDesc *desc, int32_t len)
  {
  FATFile *self = desc->self;
  FRESULT fr = f_lseek (&(self->fp), (FSIZE_t)len);
  if (fr == 0)
    fr = f_truncate (&(self->fp));
  return -fat_fresult_to_error (fr);
  }

/*============================================================================
 * fatfile_get_mtime
 * ==========================================================================*/
time_t fatfile_get_mtime (FileDesc *desc)
  {
  //FATFile *self = desc->self;
  FILINFO fno;
  f_stat (desc->name, &fno);
  return fat_time_to_unix (fno.fdate << 16 | fno.ftime);
  }

/*============================================================================
 * fatfile_new
 * ==========================================================================*/
FATFile *fatfile_new (const char *path, int flags)
  {
  FATFile *self = malloc (sizeof (FATFile));
  FileDesc *filedesc = filedesc_new ();
  filedesc->name = strdup (path);
  filedesc->close = fatfile_close;
  filedesc->open = fatfile_open;
  filedesc->read = fatfile_read;
  filedesc->write = fatfile_write;
  filedesc->read_timeout = fatfile_read_timeout;
  filedesc->get_size = fatfile_get_size;
  filedesc->get_mtime = fatfile_get_mtime;
  filedesc->lseek = fatfile_lseek;
  filedesc->truncate = fatfile_truncate;
  filedesc->self = self;
  self->filedesc = filedesc;
  self->flags = flags;
  return self;
  }

/*============================================================================
 * fatfile_destroy
 * ==========================================================================*/
void fatfile_destroy (FATFile *self)
  {
  free (self->filedesc->name);
  free (self->filedesc);
  free (self);
  }

/*============================================================================
 * fatfile_get_filedesc
 * ==========================================================================*/
FileDesc *fatfile_get_filedesc (FATFile *self)
  {
  return self->filedesc;
  }




