/*============================================================================
 *  fsmanager/devfsdir.c
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
#include <sys/clocks.h>
#include <devfs/devfsdir.h>
#include <devfs/devfs.h>

struct _DevFSDir
  {
  int pos;
  FileDesc *filedesc;
  const DevFS *devfs; // The owner of this dir listing
  };

/*============================================================================
 * devfsdir_close
 * ==========================================================================*/
Error devfsdir_close (FileDesc *descriptor)
  {
  (void)descriptor;
  DevFSDir *self = descriptor->self;
  devfsdir_destroy (self);
  return 0;
  }

/*============================================================================
 * devfsdir_open
 * ==========================================================================*/
Error devfsdir_open (FileDesc *descriptor, int flags)
  {
  (void)flags;
  DevFSDir *self = descriptor->self;
  self->pos = 0;
  descriptor->type = S_IFDIR;
  
  return 0; // Always succeeds
  }

/*============================================================================
 * devfsdir_write
 * ==========================================================================*/
void devfsdir_write (FileDesc *descriptor, int c)
  {
  (void)descriptor; (void)c;
  }

/*============================================================================
 * devfsdir_read
 * ==========================================================================*/
int devfsdir_read (FileDesc *descriptor, void *buffer, int len)
  {
  DevFSDir *self = descriptor->self;
  if (self->pos >= devfs_get_device_count (self->devfs)) return 0;
  DirEntry de;
  strncpy (de.name, devfs_get_device_name (self->devfs, self->pos), PATH_MAX);
  de.type = S_IFCHR;
  de.size = 0;
  de.mtime = clocks_get_time();
  memcpy (buffer, &de, (size_t)len);
  self->pos++; 
  return len;
  }

/*============================================================================
 * devfsdir_read_timeout
 * ==========================================================================*/
int devfsdir_read_timeout (FileDesc *descriptor, int msec)
  {
  (void)descriptor; (void)msec;
  return ENOSYS;
  }

/*============================================================================
 * devfsdir_get_size
 * ==========================================================================*/
int32_t devfsdir_get_size (FileDesc *descriptor)
  {
  (void)descriptor; 
  return 0;
  }

/*============================================================================
 * devfsdir_new
 * ==========================================================================*/
DevFSDir *devfsdir_new (const DevFS *devfs, const char *path)
  {
  DevFSDir *self = malloc (sizeof (DevFSDir));
  self->devfs = devfs;
  FileDesc *filedesc = filedesc_new ();
  filedesc->name = strdup (path);
  filedesc->close = devfsdir_close;
  filedesc->open = devfsdir_open;
  filedesc->read = devfsdir_read;
  filedesc->read_timeout = devfsdir_read_timeout;
  filedesc->get_size = devfsdir_get_size;
  filedesc->self = self;
  self->filedesc = filedesc;
  return self;
  }

/*============================================================================
 * devfsdir_destroy
 * ==========================================================================*/
void devfsdir_destroy (DevFSDir *self)
  {
  free (self->filedesc->name);
  free (self->filedesc);
  free (self);
  }

/*============================================================================
 * devfsdir_get_filedesc
 * ==========================================================================*/
FileDesc *devfsdir_get_filedesc (DevFSDir *self)
  {
  return self->filedesc;
  }


