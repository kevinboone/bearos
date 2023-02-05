/*============================================================================
 *  fsmanager/devfs.c
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
#include <sys/fsysdesc.h>
#include <errno.h>
#include <devfs/devfs.h>
#include <devfs/devfsdir.h>
#include <chardev/i2clcddev.h>
#include <devmgr/devmgr.h>

struct _DevFS
  {
  FSysDescriptor *descriptor;
  };


/*============================================================================
 * devfs_mount 
 * ==========================================================================*/
Error devfs_mount (FSysDescriptor *descriptor)
  {
  (void)descriptor;
  return 0;
  }

/*============================================================================
 * devfs_unmount 
 * ==========================================================================*/
Error devfs_unmount (FSysDescriptor *descriptor)
  {
  (void)descriptor;
  return 0;
  }

/*============================================================================
 * devfs_get_capacity
 * ==========================================================================*/
Error devfs_get_capacity (FSysDescriptor *descriptor, 
         int32_t *total_sectors, int32_t *free_sectors)
  {
  (void)descriptor;
  *total_sectors = 0;
  *free_sectors = 0;
  return 0;
  }

/*============================================================================
 * devfs_get_filedesc
 * ==========================================================================*/
FileDesc *devfs_get_filedesc (FSysDescriptor *descriptor, const char *path, 
      int flags, Error *error)
  {
  (void)flags;

  DevFS *self = descriptor->self;   
  FileDesc *ret = NULL;
  *error = 0;

  DevDescriptor *dev_desc = devmgr_find_descriptor (path + 1);
  if (dev_desc)
    {
    return dev_desc->get_file_desc (dev_desc);
    }
  else if (strcmp (path, "/") == 0)
    {
    DevFSDir *devfsdir = devfsdir_new (self, path);
    FileDesc *filedesc = devfsdir_get_filedesc (devfsdir);
    Error err = filedesc->open (filedesc, flags);
    if (err == 0)
      {
      ret = filedesc;
      *error = 0;
      }
    else
      {
      devfsdir_destroy (devfsdir); 
      *error = err;
      }
    }
  else
    *error = ENOENT;

  return ret;
  }

/*============================================================================
 * devfs_new
 * ==========================================================================*/
DevFS *devfs_new ()
  {
  DevFS *self = malloc (sizeof (DevFS));
  FSysDescriptor *descriptor = malloc (sizeof (FSysDescriptor));
  descriptor->mount = devfs_mount;
  descriptor->unmount = devfs_unmount;
  descriptor->get_filedesc = devfs_get_filedesc;
  descriptor->get_capacity = devfs_get_capacity;
  descriptor->self = self;
  descriptor->name = strdup ("Devices");
  self->descriptor = descriptor;
  return self;
  }

/*============================================================================
 * devfs_destroy
 * ==========================================================================*/
void devfs_destroy (DevFS *self)
  {
  free (self->descriptor->name);
  free (self->descriptor);
  free (self);
  }

/*============================================================================
 * devfs_get_descriptor
 * ==========================================================================*/
FSysDescriptor *devfs_get_descriptor (const DevFS *self)
  {
  return self->descriptor;
  }

/*============================================================================
 * devfs_get_device_count
 * ==========================================================================*/
int devfs_get_device_count (const DevFS *self)
  {
  (void)self;
  return devmgr_get_num_devs(); 
  }

/*============================================================================
 * devfs_get_device_name
 * ==========================================================================*/
const char *devfs_get_device_name (const DevFS *self, int n)
  {
  (void)self;
  DevDescriptor *desc = devmgr_get_desc (n);
  if (desc)
    return desc->name;
  return "?";
  }


