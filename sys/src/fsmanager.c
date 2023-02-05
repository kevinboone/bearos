/*============================================================================
 *  fsmanager/fsmanager.c
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <sys/fsmanager.h>
#include <sys/process.h>

static FSysDescriptor *mounts [FSMANAGER_MAX_MOUNTS];

/*============================================================================
 * fsmanager_init
 * ==========================================================================*/
void fsmanager_init (void)
  {
  for (int i = 0; i < FSMANAGER_MAX_MOUNTS; i++)
    mounts[i] = NULL;
  }

/*============================================================================
 * fsmanager_mount
 * ==========================================================================*/
Error fsmanager_mount (int8_t drive, FSysDescriptor *descriptor)
  {
  if (drive < 0 || drive >= FSMANAGER_MAX_MOUNTS)
    return EINVAL;

  if (mounts[drive] != NULL)
    return EINVAL;

  Error err = descriptor->mount (descriptor);
  if (err == 0)
    mounts[drive] = descriptor;

  return err;
  }

/*============================================================================
 * fsmanager_unmount
 * ==========================================================================*/
Error fsmanager_unmount (int8_t drive)
  {
  if (drive < 0 || drive >= FSMANAGER_MAX_MOUNTS)
    return EINVAL;

  if (mounts[drive] == NULL)
    return EINVAL;
 
  mounts[drive]->unmount (mounts[drive]);
  mounts[drive] = NULL;
  return 0;
  }

/*============================================================================
 * fsmanager_get_descriptor
 * ==========================================================================*/
FSysDescriptor *fsmanager_get_descriptor (int8_t drive)
  {
  if (drive < 0 || drive >= FSMANAGER_MAX_MOUNTS)
    return NULL;

  return (mounts[drive]);
  }

/*============================================================================
 * fsmanager_get_descriptor_by_path
 * ==========================================================================*/
FSysDescriptor *fsmanager_get_descriptor_by_path (const char *path)
  {
  int l = (int)strlen (path);
  int drive = -1;
  if (l >= 2)
    {
    if (path[1] == ':')
      {
      drive = toupper (path[0]) - 'A';
      if (drive < 0 || drive > FSMANAGER_MAX_MOUNTS) return NULL;
      }
    }
  
  Process *p = process_get_current();
  if (drive < 0)
    drive = process_get_current_drive (p);

  // TODO check current directory

  return fsmanager_get_descriptor ((int8_t)drive);
  }




