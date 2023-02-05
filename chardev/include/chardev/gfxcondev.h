/*============================================================================
 *  chardev/gfxcondev.h
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

#pragma once

/*============================================================================
 * ==========================================================================*/

#include <stdint.h>
#include <sys/error.h>
#include <devmgr/devmgr.h>

struct _GfxConDev;
typedef struct _GfxConDev GfxConDev;

#ifdef __cplusplus
extern "C" {
#endif

extern GfxConDev *gfxcondev_new (const char *name, DevDescriptor *gfxdevdesc);
extern void gfxcondev_destroy (GfxConDev *self);
extern DevDescriptor *gfxcondev_get_desc (GfxConDev *self); 
extern void gfxcondev_adjust_to_hardware (GfxConDev *self);

/** Clears the screen, homes the cursor, and sets all flags to default. */
extern void gfxcondev_full_reset (GfxConDev *self);

/** Clear screen without moving cursor.
'what' control what is cleared -- 0 = from cursor, 1 = to cursor, 2 = all.
what = 3 clears the scroll-back buffer, if the device supports this. */
extern void gfxcondev_clear_screen (GfxConDev *self, int what);

extern void gfxcondev_cursor_show (const GfxConDev *self, bool visible);

// Note that these cursor-movement functions don't move the hardware cursor,
//   only the driver's internal idea of where the customer is. It's usually
//   necessary to wrap these calls in calls to gfxcondev_cursor_show()
extern void gfxcondev_cursor_up (GfxConDev *self, int rows);
extern void gfxcondev_cursor_down (GfxConDev *self, int rows);
extern void gfxcondev_cursor_back (GfxConDev *self, int rows);
extern void gfxcondev_cursor_forward (GfxConDev *self, int rows);
extern void gfxcondev_set_cursor (GfxConDev *self, int row, int col);

#ifdef __cplusplus
}
#endif






