/*============================================================================
 *  waveshare_lcd/waveshare_lcd.c
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/
#pragma once

#if PICO_ON_DEVICE
#include <hardware/spi.h>
#endif 

#define	WSLCD_COLOR  uint16_t
#define	WSLCD_POINT uint16_t
#define	WSLCD_LENGTH uint16_t

/*============================================================================
 * scan directions
 * ==========================================================================*/
typedef enum _WSLCDScanDir 
  {
  WSLCD_SCAN_NORMAL  = 0,
  WSLCD_SCAN_INVERTED
  } WSLCDScanDir; 

typedef struct _WSLCD WSLCD;

#if PICO_ON_DEVICE
extern WSLCD *wslcd_new (spi_inst_t *spi, uint gpio_cs, uint gpio_miso, 
    uint gpio_mosi, uint gpio_sck, uint gpio_rst, uint gpio_dc, 
    uint gpio_bl, int baud_rate, WSLCDScanDir scan_dir);
#else
extern WSLCD *wslcd_new (void);
#endif

extern void wslcd_destroy (WSLCD *self);

extern void wslcd_init (WSLCD *self);

extern int wslcd_get_width (const WSLCD *self);
extern int wslcd_get_height (const WSLCD *self);

/** Fills an area with a specified RGB565 color. Note that the paramters
    are the start and end points, not start point at size. Also, the 
    arguments are inclusive in start and exclusive in end. That is, the 
    end coordinates are the first row and column that are _not_ filled. */
extern void wslcd_fill_area (const WSLCD *self, uint16_t xstart, 
         uint16_t ystart, uint16_t xend, uint16_t yend,        
         uint16_t colour);

extern void wslcd_set_pixel (const WSLCD *self, uint16_t x, uint16_t y, 
      uint16_t colour);

extern void wslcd_clear (WSLCD *self, uint16_t colour);

extern void wslcd_transfer (const WSLCD *self, const uint16_t *buff, int len);

extern void wslcd_transfer_window 
        (const WSLCD *self, const uint16_t *buff, uint16_t w, 
        uint16_t h, uint16_t x, uint16_t y);

extern void wslcd_set_window (const WSLCD *self, uint16_t xstart, 
         uint16_t ystart, uint16_t xend, uint16_t yend);

extern void wslcd_send_repeated_word (const WSLCD *self, uint16_t word, 
         int len);

extern void wslcd_scroll_up (WSLCD *self, uint16_t pixels);

extern void wslcd_set_used_area (WSLCD *self, int width, int height);


