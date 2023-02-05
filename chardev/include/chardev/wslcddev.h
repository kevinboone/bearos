/*============================================================================
 *  chardev/wslcddev.h
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

#pragma once

/*============================================================================
 * ==========================================================================*/

#include <stdint.h>
#include <sys/error.h>
#include <devmgr/devmgr.h>
#include <waveshare_lcd/waveshare_lcd.h>

#if PICO_ON_DEVICE
#include <hardware/spi.h>
#endif 

struct _WSLCDDev;
typedef struct _WSLCDDev WSLCDDev;

#ifdef __cplusplus
extern "C" {
#endif

extern WSLCDDev *wslcddev_new (const char *name);
extern void wslcddev_destroy (WSLCDDev *self);
extern DevDescriptor *wslcddev_get_desc (WSLCDDev *self); 

#if PICO_ON_DEVICE
extern void wslcddev_init (WSLCDDev *self, spi_inst_t *spi, uint gpio_cs, 
    uint gpio_miso, uint gpio_mosi, uint gpio_sck, uint gpio_rst, uint gpio_dc, 
    uint gpio_bl, int baud_rate, WSLCDScanDir scan_dir);
#else
extern void wslcddev_init (WSLCDDev *self);
#endif

#ifdef __cplusplus
}
#endif







