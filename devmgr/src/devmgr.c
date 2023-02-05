/*============================================================================
 *  devmgr/devmgr.c
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

/*============================================================================
 * ==========================================================================*/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <pico/stdlib.h>
#include <stdio.h>
#include <devmgr/devmgr.h>

static int numdevs = 0;

static DevDescriptor *devs[DEVMGR_MAX_DEVS];

/*============================================================================
 * devmgr_init 
 * ==========================================================================*/
void devmgr_init (void)
  {
  memset (devs, 0, sizeof (devs));
  }

/*============================================================================
 * devmgr_deinit 
 * ==========================================================================*/
void devmgr_deinit (void)
  {
  }

/*============================================================================
 * devmgr_deinit 
 * ==========================================================================*/
void devmgr_register (DevDescriptor *desc)
  {
  // TODO check bounds
  devs[numdevs] = desc;
  numdevs++;
  }

/*============================================================================
 * devmgr_find_descriptor
 * ==========================================================================*/
DevDescriptor *devmgr_find_descriptor (const char *name)
  {
  for (int i = 0; i < DEVMGR_MAX_DEVS; i++)
    {
    if (devs[i])
      {
      if (strcasecmp (devs[i]->name, name) == 0) return devs[i];
      }
    }
  return 0;
  }

/*============================================================================
 * devmgr_get_num_devs
 * ==========================================================================*/
int devmgr_get_num_devs (void)
  {
  return numdevs;
  }

/*============================================================================
 * devmgr_get_desc
 * ==========================================================================*/
DevDescriptor *devmgr_get_desc (int n)
  {
  if (n >= 0 && n < DEVMGR_MAX_DEVS)
    return devs[n];
  else
    return 0;
  }


