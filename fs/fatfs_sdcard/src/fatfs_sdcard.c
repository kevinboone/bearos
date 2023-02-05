/*============================================================================
 * fatfs_sdcard/fatfs_sdcard.c
 *
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 *
 * ==========================================================================*/

#if PICO_ON_DEVICE

#include <stdio.h>
#include <ff.h> // From ChaN's FAT driver
#include <diskio.h> // From ChaN's FAT driver
#include <sdcard/sdcard.h> 
#include <syslog/syslog.h>
#include <sys/clocks.h>
#include <fat/fat.h>

#define TRACE SYSLOG_TRACE
#define DEBUG SYSLOG_DEBUG
#define INFO SYSLOG_INFO
#define WARN SYSLOG_WARN


/*============================================================================
 * sdcard_err_to_fatfs_err
 * Converts and error code returned by one of the sdcard_xxx functions into
 *   the corresponding error code required by the FAT driver. Note that
 *   the sdcard error codes are more fine-grained, and many codes map onto
 *   the same FAT error 
 * ==========================================================================*/
static int sdcard_err_to_fatfs_err (int err) 
  {
  switch (err) 
    {
    case 0:
	return RES_OK;
    case SD_ERR_UNUSABLE:
    case SD_ERR_NO_RESPONSE:
    case SD_ERR_UNINIT_DRIVER:
    case SD_ERR_UNINIT_CARD:
    case SD_ERR_NO_DEVICE:
	return RES_NOTRDY;
    case SD_ERR_PARAMETER:
    case SD_ERR_UNSUPPORTED:
	return RES_PARERR;
    case SD_ERR_WRITE_PROTECTED:
	return RES_WRPRT;
    case SD_ERR_CRC:
    case SD_ERR_ERASE:
    case SD_ERR_WRITE:
    default:
	  return RES_ERROR;
    }
  }

/*============================================================================
 * disk_initialize
 * see http://elm-chan.org/fsw/ff/doc/dinit.html
 * ==========================================================================*/
DSTATUS disk_initialize (BYTE pdrv)
  {
#ifdef TRACE
  TRACE ("%s:%d start", __FUNCTION__, __LINE__);
#endif

  DSTATUS ret = 0;
  SDCard *s = sdcard_get_instance (pdrv); 
  if (sdcard_is_driver_initialized (s))
    {
    SDError err = sdcard_insert_card (s); 
#ifdef WARN
    if (err != 0)
      WARN ("%s:%d sdcard_insert error %d", __FUNCTION__, __LINE__, err);
#endif

    if (sdcard_is_card_initialized (s))
      {
#ifdef TRACE
      TRACE ("%s:%d card appears to be initialized", __FUNCTION__, __LINE__);
#endif
      }
    else
      ret |= STA_NODISK;
    }
  else
    ret |= STA_NOINIT;

#ifdef TRACE
  TRACE ("%s:%d donn", __FUNCTION__, __LINE__);
#endif
    return ret; 
  }


/*============================================================================
 * disk_status
 * see http://elm-chan.org/fsw/ff/doc/dstat.html
 * ==========================================================================*/
DSTATUS disk_status (BYTE pdrv)
  {
#ifdef TRACE
  TRACE ("%s:%d start", __FUNCTION__, __LINE__);
#endif
  SDCard *s = sdcard_get_instance (pdrv); 
  if (!s)
    {
#ifdef WARN
  WARN ("%s:%d sdcard library not initialized", __FUNCTION__, __LINE__);
#endif
    return RES_PARERR;
    }
  DSTATUS ret = 0;
  if (sdcard_is_driver_initialized (s))
    {
    if (sdcard_is_card_initialized (s))
      {
#ifdef TRACE
      TRACE ("%s:%d card appears to be initialized", __FUNCTION__, __LINE__);
#endif
      }
    else
      ret |= STA_NODISK;
    }
  else
    ret |= STA_NOINIT;

  /* ChaN's FAT driver understands the concept of a write-protected drive. 
     However, my SD card driver does not report write-protected status,
     because most modern SD cards do not have the physical switch any more. */

#ifdef TRACE
  TRACE ("%s:%d done", __FUNCTION__, __LINE__);
#endif
  return ret; 
  }

/*============================================================================
 * disk_read
 * see http://elm-chan.org/fsw/ff/doc/dread.html
 * ==========================================================================*/
DRESULT disk_read (BYTE pdrv,  BYTE *buff, LBA_t sector, UINT count) 
  {
#ifdef TRACE
  TRACE ("%s:%d start", __FUNCTION__, __LINE__);
#endif

  SDCard *s = sdcard_get_instance (pdrv); 
  if (!s)
    return RES_NOTRDY; // Should not be possible to get here

  SDError ret = sdcard_read_sectors (s, buff, (uint32_t)sector, count);

#ifdef WARN
  if (ret != 0)
      WARN ("%s:%d sdcard_read_sectors error %d", __FUNCTION__, 
        __LINE__, ret);
#endif
  
#ifdef TRACE
  TRACE ("%s:%d done", __FUNCTION__, __LINE__);
#endif
  return sdcard_err_to_fatfs_err (ret);
  }

/*============================================================================
 * disk_write
 * see http://elm-chan.org/fsw/ff/doc/dwrite.html
 * ==========================================================================*/
DRESULT disk_write (BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count) 
  {
#ifdef TRACE
  TRACE ("%s:%d start", __FUNCTION__, __LINE__);
#endif

  SDCard *s = sdcard_get_instance (pdrv); 
  if (!s)
    return RES_NOTRDY; // Should not be possible to get here

  SDError ret = sdcard_write_sectors (s, buff, (uint32_t)sector, count);

#ifdef WARN
  if (ret != 0)
      WARN ("%s:%d sdcard_read_sectors error %d", __FUNCTION__, 
        __LINE__, ret);
#endif
  
#ifdef TRACE
  TRACE ("%s:%d done", __FUNCTION__, __LINE__);
#endif
  return sdcard_err_to_fatfs_err (ret);
  }

DRESULT disk_ioctl (BYTE pdrv, BYTE cmd, void *buff)
  {
  (void)pdrv; (void)cmd; (void)buff;
  return 0;
  }

DWORD get_fattime (void)
  {
  return fat_unix_to_time (clocks_get_time());
  return 0;
  }

#endif

