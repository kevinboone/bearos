/*============================================================================
 * fatfs_loopback/fatfs_loopback.c
 *
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 *
 * ==========================================================================*/

#if PICO_ON_DEVICE
#else

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <ff.h> // From ChaN's FAT driver
#include <diskio.h> // From ChaN's FAT driver
#include <syslog/syslog.h>
#include <sys/clocks.h>
#include <fat/fat.h>

#define TRACE SYSLOG_TRACE
#define DEBUG SYSLOG_DEBUG
#define INFO SYSLOG_INFO
#define WARN SYSLOG_WARN

#define FATFS_LOOPBACK_FILE "/tmp/fatfs_loopback.img"

static int fd = -1;

/*============================================================================
 * sdcard_err_to_fatfs_err
 * Converts and error code returned by one of the sdcard_xxx functions into
 *   the corresponding error code required by the FAT driver. Note that
 *   the sdcard error codes are more fine-grained, and many codes map onto
 *   the same FAT error 
 * ==========================================================================*/
static int linux_err_to_fatfs_err (int err) 
  {
  if (err == 0) return 0;
  return RES_ERROR;
  }

/*============================================================================
 * disk_initialize
 * see http://elm-chan.org/fsw/ff/doc/dinit.html
 * ==========================================================================*/
DSTATUS disk_initialize (BYTE pdrv)
  {
  (void)pdrv;
#ifdef TRACE
  TRACE ("Start");
#endif

  DSTATUS ret = 0;
  fd = open (FATFS_LOOPBACK_FILE, O_RDWR);
  if (fd < 0)
    return STA_NOINIT;

#ifdef TRACE
  TRACE ("Done");
#endif

  return ret; 
  }


/*============================================================================
 * disk_status
 * see http://elm-chan.org/fsw/ff/doc/dstat.html
 * ==========================================================================*/
DSTATUS disk_status (BYTE pdrv)
  {
  (void)pdrv;
#ifdef TRACE
  TRACE ("Start");
#endif

  DSTATUS ret = 0;

#ifdef TRACE
  TRACE ("Done");
#endif
  return ret; 
  }

/*============================================================================
 * disk_read
 * see http://elm-chan.org/fsw/ff/doc/dread.html
 * ==========================================================================*/
DRESULT disk_read (BYTE pdrv,  BYTE *buff, LBA_t sector, UINT count) 
  {
  (void)pdrv;
#ifdef TRACE
  TRACE ("Start sector=%llu, count=%u", sector, count);
#endif
  int ret = 0;

  lseek (fd, (long int)sector * 512, SEEK_SET);
  read (fd, buff, count * 512);

#ifdef TRACE
  TRACE ("Done");
#endif
  return linux_err_to_fatfs_err (ret);
  }

/*============================================================================
 * disk_write
 * see http://elm-chan.org/fsw/ff/doc/dwrite.html
 * ==========================================================================*/
DRESULT disk_write (BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count) 
  {
  (void)pdrv;
#ifdef TRACE
  TRACE ("Done");
#endif
  int ret = 0;

  lseek (fd, (long int)sector * 512, SEEK_SET);
  write (fd, buff, count * 512);
  
#ifdef TRACE
  TRACE ("Done");
#endif
  return linux_err_to_fatfs_err (ret);
  }

DRESULT disk_ioctl (BYTE pdrv, BYTE cmd, void *buff)
  {
  (void)pdrv; (void)cmd; (void)buff;
  return 0;
  }

DWORD get_fattime (void)
  {
  return fat_unix_to_time (clocks_get_time());
  }

#endif

