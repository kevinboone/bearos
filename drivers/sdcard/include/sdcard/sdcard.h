/*============================================================================
 *  sdcard/sdcard.h
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

#pragma once

#if PICO_ON_DEVICE

#include <stdint.h>
#include <hardware/gpio.h>
#include <hardware/spi.h>

/*============================================================================
 * ==========================================================================*/

typedef int SDError;
// SD_ERR_ERASE for completeness -- this implementation cannot raise this err
#define SD_ERR_ERASE           -1001  
#define SD_ERR_UNSUPPORTED     -1002
#define SD_ERR_CRC             -1003
#define SD_ERR_PARAMETER       -1004
#define SD_ERR_WRITE_PROTECTED -1005
#define SD_ERR_NO_DEVICE       -1006
#define SD_ERR_WRITE           -1007
#define SD_ERR_UNUSABLE        -1008
// Card does not support CRC
#define SD_ERR_NO_CRC          -1009
// Timed out waiting for response
#define SD_ERR_NO_RESPONSE     -1010
// Driver has not been initialized
#define SD_ERR_UNINIT_DRIVER   -1011
// Card has not been initialized
#define SD_ERR_UNINIT_CARD     -1012
#define SD_ERR_SEC_RANGE       -1013

// We will always use 512-byte reads and writes
#define SD_BLOCK_SIZE 512

/** SD/MMC card types  */
typedef enum 
  {
  SDCARD_NONE = 0,
  /** v1.x Standard Capacity */
  SDCARD_V1  = 1,    
  /** v2.x Standard capacity */
  SDCARD_V2  = 2,    
  /** v2.x High capacity */
  SDCARD_V2HC = 3,  
  /** Unknown or unsupported card */
  SDCARD_UNKNOWN = 4 
  } SDCardType;

typedef struct _SDCard SDCard;

#ifdef __cplusplus
extern "C" {
#endif

/** Create a new SDCard object, specifying the pins to which the various
      SDCard terminals are connected, the SPI bus (spi0 or spi1) to use,
      the GPIO drive strength, and the baud rate. GPIO drive strength is
      one of the GPIO_DRIVE_STRENGTH constants, and can be set to
      zero to indicate "don't set".
    NOTE: the numbers for gpio_cs are GPIO numbers, not package pin
      numbers. It's very easy to get this wrong. */
extern SDCard *sdcard_new (spi_inst_t *spi, int drive_strength, uint gpio_cs,
          uint gpio_miso, uint gpio_mosi, uint gpio_sck, int baud_rate);

/** Clean up the SDCard driver. Note that we can free the memory used,
      but we can't uninitalize the low-level hardware settings, because we
      don't know what they were. */
extern void sdcard_destroy (SDCard* self);

/** Initialize the driver, but not the card. This should usually be the
      first function called on the SDCard object after creation. */
extern SDError sdcard_init (SDCard *self);

/** Initialize the card. This method can fail in a variety of ways --
      the caller must check the return value. Apart from the waste of
      time, it won't hurt to call insert_card more than once on the
      same card. Even if the card has been initialized, it will be
      initialized again. So we can reinitialize if we think a different
      card might have been inserted. */ 
extern SDError sdcard_insert_card (SDCard *self);

/** Inactivate the card. In practice, this does nothing -- it just marks
    the card as needing reinitialization. */
extern SDError sdcard_eject_card (SDCard *self);

/**  Get the card type -- set by sdcard_insert (if it succeeds) */
extern SDCardType sdcard_get_type (const SDCard *self);

/** Get the sector count -- set by sdcard_insert (if it succeeds) */
extern uint64_t sdcard_get_sectors (const SDCard *self);

/** Read one or more sectors from the card into memory. The card and
     the driver must first have been initialized. The caller _must_
     check the return value to see whether the read succeeded. */
extern SDError sdcard_read_sectors (SDCard *self, uint8_t *buffer, 
          uint32_t start, uint32_t count);

/** Write one or more sectors to the card from memory. The card and
     the driver must first have been initialized. The caller _must_
     check the return value to see whether the read succeeded. */
extern SDError sdcard_write_sectors (SDCard *self, const uint8_t *buffer, 
         uint32_t start, uint32_t count);

/** Get a human-readable string representing the result from get_card_type. */
extern const char *sdcard_type_to_string (SDCardType type);

/** Get a string representation (which need not be freed) of the
     specified error code (one of the SD_ERR_XXX values) */
extern const char *sdcard_perror (SDError error);

/** Get the SDCard instance that represents a particular drive. Note that,
    at present, only one drive is supported, and the drive_num argument
    is ignore. */
extern SDCard *sdcard_get_instance (int drive_num);

/** Returns whether the driver is intialized, that is, whether
    init() was called successfully. */
extern bool sdcard_is_driver_initialized (const SDCard *self);

/** Returns whether the card is intialized, that is, whether
    insert() was called successfully. */
extern bool sdcard_is_card_initialized (const SDCard *self);

#ifdef __cplusplus
}
#endif

#endif // PICO_ON_DEVICE


