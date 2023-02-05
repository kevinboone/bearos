/*============================================================================
 *  fatfs_loopback/fatloopback.c
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

#if PICO_ON_DEVICE
#else

/*============================================================================
 * ==========================================================================*/

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/error.h>
#include <errno.h>
#include <fatfs_loopback/fatloopback.h>
#include <sys/process.h>
#include <syslog/syslog.h>
#include <fat/fat.h>
#include <fat/fatdir.h>
#include <fat/fatfile.h>
#include <ff.h>

#define TRACE_IN SYSLOG_TRACE_IN
#define TRACE_OUT SYSLOG_TRACE_OUT
#define TRACE SYSLOG_TRACE
#define DEBUG SYSLOG_DEBUG
#define INFO SYSLOG_INFO
#define WARN SYSLOG_WARN

struct _FatLoopback
  {
  bool mounted;
  FSysDescriptor *descriptor;
  FATFS fatfs;
  };

/*============================================================================
 * fatloopback_mount 
 * ==========================================================================*/
Error fatloopback_mount (FSysDescriptor *descriptor)
  {
  Error ret = 0;
  FatLoopback *self = descriptor->self;

  if (!self->mounted)
    {
    FRESULT fr = f_mount (&(self->fatfs), "0:", 1); // TODO
    if (fr)
      {
      ret = fat_fresult_to_error (fr);
      }
    else
      self->mounted = true;
    }

  return ret;
  }

/*============================================================================
 * fatloopback_get_capacity
 * ==========================================================================*/
Error fatloopback_get_capacity (FSysDescriptor *descriptor, 
         int32_t *total_sectors, int32_t *free_sectors)
  {
  (void)descriptor;
  Error ret = 0;
  FATFS *ff;
  unsigned int n;
  FRESULT fr = f_getfree ("0:", &n, &ff); // TODO
  if (fr)
    {
    ret = fat_fresult_to_error (fr);
    }
  else
    {
    *free_sectors = (int32_t)(ff->csize * n);
    *total_sectors = (int32_t)(ff->n_fatent - 2) * ff->csize;
    }

  return ret;
  }

/*============================================================================
 * fatloopback_unmount 
 * ==========================================================================*/
Error fatloopback_unmount (FSysDescriptor *descriptor)
  {
  FatLoopback *self = descriptor->self;
  f_unmount ("0:"); // TODO
  self->mounted = false;
  return 0;
  }

/*============================================================================
 * fatloopback_get_filedesc_file
 * ==========================================================================*/
FileDesc *fatloopback_get_filedesc_file (FSysDescriptor *descriptor, 
      const char *path, int flags, Error *error)
  {
  (void)descriptor;
  FileDesc *ret = NULL;
  // Again, we have to do something about drive number here...
  FATFile *fatfile = fatfile_new (path, flags);
  FileDesc *filedesc = fatfile_get_filedesc (fatfile);
  Error err = filedesc->open (filedesc, flags);
  if (err == 0)
    {
    ret = filedesc;
    *error = 0;
    }
  else
    {
    fatfile_destroy (fatfile); 
    *error = err;
    }
  return ret;
  }

/*============================================================================
 * fatloopback_get_filedesc_dir
 * ==========================================================================*/
FileDesc *fatloopback_get_filedesc_dir (FSysDescriptor *descriptor, 
      const char *path, int flags, Error *error)
  {
  (void)descriptor; (void)flags;
  FileDesc *ret = NULL;
  // Again, we have to do something about drive number here...
  FATDir *fatdir = fatdir_new (path);
  FileDesc *filedesc = fatdir_get_filedesc (fatdir);
  Error err = filedesc->open (filedesc, 0);
  if (err == 0)
    {
    ret = filedesc;
    *error = 0;
    }
  else
    {
    fatdir_destroy (fatdir); 
    *error = err;
    }
  return ret;
  }

/*============================================================================
 * fatloopback_get_filedesc
 * ==========================================================================*/
FileDesc *fatloopback_get_filedesc (FSysDescriptor *descriptor, 
      const char *path, int flags, Error *error)
  {
  (void)descriptor;
#ifdef DEBUG
  DEBUG ("path=%s flags=%04X", path);
#endif

  *error = 0;

  FILINFO finfo;
  // TODO -- how do we handle drives here? Perhaps we need to
  //   reinstate the drive letter? Not a problem when there is ony
  //   one FAT drive

  char _path[PATH_MAX];
  strncpy (_path, path, PATH_MAX);
  fat_remove_trailing_slash (_path);

  // Note that the FAT drive does not support calling f_stat on the root
  //    directory
  if (strcmp (_path, "/") == 0)
    {
    return fatloopback_get_filedesc_dir (descriptor, _path, flags, error);
    }

  FRESULT fr = f_stat (_path, &finfo);	
  if (fr == 0)
    {
    if (finfo.fattrib & AM_DIR)
      {
      return fatloopback_get_filedesc_dir (descriptor, _path, flags, error);
      }
    }
  return fatloopback_get_filedesc_file (descriptor, _path, flags, error);
  }

/*============================================================================
 * fatloopback_mkdir
 * ==========================================================================*/
Error fatloopback_mkdir (FSysDescriptor *desc, const char *path)
  {
  (void)desc;
  return fat_fresult_to_error (f_mkdir (path));
  }

/*============================================================================
 * fatloopback_rename
 * ==========================================================================*/
Error fatloopback_rename (FSysDescriptor *desc, const char *source, 
        const char *target)
  {
  (void)desc;
  return fat_fresult_to_error (f_rename (source, target));
  }

/*============================================================================
 * fatloopback_unlink
 * ==========================================================================*/
Error fatloopback_unlink (FSysDescriptor *desc, const char *path)
  {
  (void)desc;
  return fat_fresult_to_error (f_unlink (path));
  }

/*============================================================================
 * fatloopback_rmdir
 * ==========================================================================*/
Error fatloopback_rmdir (FSysDescriptor *desc, const char *path)
  {
  (void)desc;
  return fat_fresult_to_error (f_unlink (path));
  }

/*============================================================================
 * fatloopback_utime
 * ==========================================================================*/
Error fatloopback_utime (FSysDescriptor *desc, 
                 const char *path, const struct utimbuf *times)
  {
  (void)desc;
  DWORD dt;
  if (times->modtime == 0)
    dt = 0;
  else
    {
    int offset = 60 * process_get_utc_offset (process_get_current());
    dt = fat_unix_to_time (times->modtime - offset);
    }
  FILINFO fno = 
    {
    .fdate = (WORD)((dt >> 16) & 0xFFFF),
    .ftime = dt & 0xFFFF 
    }; 
  return fat_fresult_to_error (f_utime (path, &fno));
  }

/*============================================================================
 * fatloopback_new
 * ==========================================================================*/
FatLoopback *fatloopback_new (void)
  {
  FatLoopback *self = malloc (sizeof (FatLoopback));
  FSysDescriptor *descriptor = malloc (sizeof (FSysDescriptor));
  descriptor->mount = fatloopback_mount;
  descriptor->unmount = fatloopback_unmount;
  descriptor->get_capacity = fatloopback_get_capacity;
  descriptor->get_filedesc = fatloopback_get_filedesc;
  descriptor->rename = fatloopback_rename;
  descriptor->mkdir = fatloopback_mkdir;
  descriptor->unlink = fatloopback_unlink;
  descriptor->rmdir = fatloopback_rmdir;
  descriptor->utime = fatloopback_utime;
  descriptor->self = self;
  descriptor->name = strdup ("FAT32 on loopback");
  self->descriptor = descriptor;
  self->mounted = false;
  return self;
  }

/*============================================================================
 * fatloopback_destroy
 * ==========================================================================*/
void fatloopback_destroy (FatLoopback *self)
  {
  free (self->descriptor->name);
  free (self->descriptor);
  free (self);
  }

/*============================================================================
 * fatloopback_get_descriptor
 * ==========================================================================*/
FSysDescriptor *fatloopback_get_descriptor (const FatLoopback *self)
  {
  return self->descriptor;
  }

#endif

