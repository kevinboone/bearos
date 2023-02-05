/*============================================================================
 *  fsmanager/fat.c
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

/*============================================================================
 * ==========================================================================*/

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/error.h>
#include <errno.h>
#include <sys/direntry.h>
#include <sys/fcntl.h>
#include <sys/datetime.h>
#include <sys/process.h>
#include <sys/syscalls.h>
#include <fat/fatdir.h>
#include <ff.h>

/*============================================================================
 * fat_fresult_to_error
 * ==========================================================================*/
Error fat_fresult_to_error (FRESULT err) 
  {
  switch (err)
    {
    case 0: return 0;
    case FR_NOT_ENOUGH_CORE:
      return ENOMEM;  
    case FR_TOO_MANY_OPEN_FILES:
      return ENFILE;  
    case FR_NOT_READY:
      return EAGAIN;  
    case FR_DISK_ERR:
    case FR_NOT_ENABLED:
    case FR_INT_ERR:
      return EIO;  
    case FR_NO_FILE:
    case FR_NO_PATH:
      return ENOENT;  
    case FR_DENIED:
    case FR_EXIST:
      return EACCES;  
    case FR_INVALID_NAME:
      return EIO;  
    case FR_INVALID_OBJECT:
    case FR_INVALID_PARAMETER:
      return EINVAL;  
    case FR_WRITE_PROTECTED:
      return EROFS;  
    case FR_INVALID_DRIVE:
      return ENOENT;  
    case FR_TIMEOUT:
      return ETIME;  
    case FR_NO_FILESYSTEM:
      return EIO;  
    default:
      return EIO;
    }
  return EIO;
  }

/*============================================================================
 * fat_remove_trailing_slash
 * ==========================================================================*/
void fat_remove_trailing_slash (char *path)
  {
  int i = (int)strlen (path) - 1;
  while (i > 1 && path[i] == '/')
    {
    path[i] = 0;
    i--;
    }
  }

/*============================================================================
 * fat_open_to_f_open_mode
 * ==========================================================================*/
uint8_t fat_open_to_f_open_mode (int flags)
  {
  uint8_t ret = 0;
  // We can't do anything with the high-numbered flags, like O_BINARY.
  // So, for now, we just mask them out.
  flags &= 0xFFFF;
  if (flags == O_RDONLY) ret |= FA_READ; 
  if (flags & O_WRONLY) ret |= FA_WRITE; 
  if (flags & O_RDWR) ret |= (FA_READ | FA_WRITE);
  if (flags & O_APPEND) ret |= FA_OPEN_APPEND;
  if (flags & O_CREAT) ret |= FA_OPEN_ALWAYS;
  if (flags & O_TRUNC) ret |= FA_CREATE_ALWAYS;
  return ret;
  }

/*============================================================================
 * fat_time_to_unix 
 * ==========================================================================*/
time_t fat_time_to_unix (DWORD ftime)
  {
  int sec = (ftime & 0x1F) << 1;
  ftime >>= 5;
  int min = (ftime & 0x3F);
  ftime >>= 6;
  int hour = (ftime & 0x1F);
  ftime >>= 5;
  int day = (ftime & 0x1F);
  ftime >>= 5;
  int month = (ftime & 0x0F);
  ftime >>= 4;
  int year = (int)ftime;

  Process *p = process_get_current();
  int utc_offset = process_get_utc_offset (p);
  return datetime_ymdhms_to_unix (year + 1980, month, 
      day, hour, min, sec, utc_offset); 
  }

/*============================================================================
 * fat_unix_to_time 
 * ==========================================================================*/
DWORD fat_unix_to_time (time_t t)
  {
  struct tm tm;
  Process *p = process_get_current();
  int utc_offset_min = process_get_utc_offset (p);
  sys_gmtime_r (t + utc_offset_min * 60, &tm);
  int year = (int)(tm.tm_year + 1900);
  int month = (int)(tm.tm_mon + 1);
  int day = (int)tm.tm_mday;
  int hour = (int)tm.tm_hour;
  int min = (int)(tm.tm_min + utc_offset_min);
  if (min > 59)
    {
    min -= 60;
    hour += 1;
    }
  else if (min < 0)
    {
    min += 60;
    hour -= 1;
    }
  int sec = tm.tm_sec;

  DWORD ret = (DWORD) (((year - 1980) << 25) | 
      ((month) << 21) |
      (day << 16) |
      (hour << 11) |
      (min << 5) |
      (sec >> 1));
  return ret;
  }




