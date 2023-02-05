/*============================================================================
 *  gfx/gfx_util.c
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/
#include <bearos/devctl.h>

/*============================================================================
 * gfx_util_get_bytes_pp 
 * ==========================================================================*/
int gfx_util_get_bytes_pp (DevCtlColMode mode)
  {
  switch (mode)
    {
    case DC_GFX_COLMODE_RGB565: return 2;
    case DC_GFX_COLMODE_RGB888: return 3;
    }
  return 0;
  }

