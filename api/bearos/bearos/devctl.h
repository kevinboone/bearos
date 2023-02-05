/*============================================================================
 *  bearos/devctl.h
 *
 * Copyright (c)2023 Kevin Boone, GPL v3.0
 * ==========================================================================*/
#pragma once

#include <bearos/types.h>
#include <stdint.h>

#define BEAROS_DEFAULT_GFX "p:/gfx"

#define DC_GET_GEN_FLAGS  0

#define DC_TERM_SET_FLAGS 1
#define DC_TERM_GET_FLAGS 2
#define DC_TERM_GET_PROPS 3 

#define DC_GFX_GET_PROPS 40
#define DC_GFX_SET_REGION 41 
#define DC_GFX_FILL 42 
#define DC_GFX_WRITE 43 
#define DC_GFX_SCROLL_UP 44 
#define DC_GFX_RESET_AND_CLEAR 45 

// Takes a DevCtlGfxProps object. Sets the number of rows and columns that
//   the user of this graphics device will actually use. This is potentially
//   important because it will prevent the driver, for example, scrolling
//   areas of the screen that are empty.
#define DC_GFX_SET_USED_AREA 46 

// General flags, could apply to all devices 
#define DC_FLAG_ISTTY  0x0001
#define DC_FLAG_ISGFX  0x0002

// Flags that apply to tty-type devices, including gfxcon devices. Note
//   that not all devices will be able to respect all flags. For example,
//   it won't be possible to change the 'wrap' mode on a remote serial 
//   console.
#define DC_TERM_FLAG_ECHO              0x0000
#define DC_TERM_FLAG_NOECHO            0x0001
#define DC_TERM_FLAG_NOWRAP            0x0000
#define DC_TERM_FLAG_WRAP              0x0002
#define DC_TERM_FLAG_LFISLF            0x0000
#define DC_TERM_FLAG_LFISCRLF          0x0004
#define DC_TERM_FLAG_BS_NONDESTRUCTIVE 0x0000
#define DC_TERM_FLAG_BS_DESTRUCTIVE    0x0080

// Colour representations for GFX device
// At present, only RGB565 is supported
typedef enum _DevCtlColMode
  {
  DC_GFX_COLMODE_RGB565 = 0,
  DC_GFX_COLMODE_RGB888 = 1 
  } DevCtlColMode;

// DevCtlTermProps is used with DC_TERM_GET_PROPS
typedef struct _DevCtlTermProps
  {
  int rows;
  int cols;
  } DevCtlTermProps;

// DevCtlGfxPropos is used with DC_GFX_GET_PROPS
typedef struct _DevCtlGfxProps
  {
  int width;
  int height;
  int colmode;
  } DevCtlGfxProps;

// DevCtlGfxPropos is used with DC_GFX_SET_REGION
typedef struct _DevCtlGfxregion
  {
  int x;
  int y;
  int cx;
  int cy;
  } DevCtlGfxRegion;

// Used by DC_GFX_FILL
typedef union _DevCtlGfxColour
  {
  uint32_t rgb888;
  uint16_t rgb565;
  uint8_t greyscale;
  } DevCtlGfxColour;

// DevCtlGfxWrite is used with DC_GFX_WRITE
typedef struct _DevCtlGfxWrite
  {
  void *buffer;
  int length; // In bytes, not pixels
  } DevCtlGfxWrite;

#ifdef __cplusplus
extern "C" {
#endif

// arg1 will nearly always be one of the DC_XXX codes
int devctl (int fd, intptr_t arg1, intptr_t arg2);

#ifdef __cplusplus
}
#endif




