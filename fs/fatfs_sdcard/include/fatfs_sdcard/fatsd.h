/*============================================================================
 *  fatfs_sdcard/fatsd.h
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

#pragma once

/*============================================================================
 * ==========================================================================*/

#if PICO_ON_DEVICE

#include <stdint.h>
#include <errno.h>
#include <hardware/spi.h>
#include <sys/fsmanager.h>

struct _FatSD;
typedef struct _FatSD FatSD;

#ifdef __cplusplus
extern "C" {
#endif

/** Create a new FatSD object, specifying the pins to which the various
      SDCard terminals are connected, the SPI bus (spi0 or spi1) to use,
      the GPIO drive strength, and the baud rate. GPIO drive strength is
      one of the GPIO_DRIVE_STRENGTH constants, and can be set to
      zero to indicate "don't set".
    NOTE: the numbers for gpio_cs are GPIO numbers, not package pin
      numbers. It's very easy to get this wrong. */
extern FatSD *fatsd_new (spi_inst_t *spi, int drive_strength, uint gpio_cs,
          uint gpio_miso, uint gpio_mosi, uint gpio_sck, int baud_rate);

extern void fatsd_destroy (FatSD *self);
extern FSysDescriptor *fatsd_get_descriptor (const FatSD *self);

#ifdef __cplusplus
}
#endif



#endif // PICO_ON_DEVICE

