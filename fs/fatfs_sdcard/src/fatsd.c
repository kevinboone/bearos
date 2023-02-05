/*============================================================================
 *  fatfs_sdcard/fatsd.c
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

#if PICO_ON_DEVICE

/*============================================================================
 * ==========================================================================*/

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <hardware/spi.h>
#include <sys/error.h>
#include <errno.h>
#include <utime.h>
#include <fatfs_sdcard/fatsd.h>
#include <syslog/syslog.h>
#include <sys/process.h>
#include <sdcard/sdcard.h>
#include <fat/fat.h>
#include <fat/fatfile.h>
#include <fat/fatdir.h>
#include <ff.h>

struct _FatSD
  {
  SDCard *sdcard;
  bool mounted;
  FSysDescriptor *descriptor;
  FATFS fatfs;
  };

/*============================================================================
 * fatsd_sderror_to_error
 * ==========================================================================*/
Error fatsd_sderror_to_error (SDError err)
  {
  if (err == 0) return 0;
  switch (err)
    {
    case SD_ERR_ERASE: 
    case SD_ERR_UNSUPPORTED: 
    case SD_ERR_CRC: 
    case SD_ERR_NO_DEVICE: 
    case SD_ERR_WRITE: 
    case SD_ERR_NO_CRC: 
    case SD_ERR_UNUSABLE: 
    case SD_ERR_NO_RESPONSE: 
      return EIO;
    case SD_ERR_SEC_RANGE: 
      return EINVAL;
    case SD_ERR_UNINIT_DRIVER: 
    case SD_ERR_UNINIT_CARD: 
      return EIO;
    case SD_ERR_WRITE_PROTECTED: 
      return EROFS;
    }
  return EIO;
  }

/*============================================================================
 * fatsd_mount 
 * ==========================================================================*/
Error fatsd_mount (FSysDescriptor *descriptor)
  {
  Error ret = 0;
  FatSD *self = descriptor->self;

  if (!sdcard_is_driver_initialized (self->sdcard))
    {
    sdcard_init (self->sdcard);
    }
  
  if (!sdcard_is_card_initialized (self->sdcard))
    {
    SDError err = sdcard_insert_card (self->sdcard);
    if (err == 0)
      {
      SDCardType ct = sdcard_get_type (self->sdcard); 
      const char *ctstring = sdcard_type_to_string (ct);
      printf ("Card type: %s\n", ctstring); 
      uint64_t sectors = sdcard_get_sectors (self->sdcard);
      printf ("Card capacity: %llu sectors, %u Mb \n", sectors, 
        (unsigned) sectors / 2048); 
      }
    else
      return fatsd_sderror_to_error (err); 
    }

  if (!self->mounted)
    {
    FRESULT fr = f_mount (&(self->fatfs), "0:", 1); // TODO
    if (fr)
      {
      printf ("Mount error %d\n", fr);
      ret = fat_fresult_to_error (fr);
      }
    else
      self->mounted = true;
    }

  return ret;
  }

/*============================================================================
 * fatsd_unmount 
 * ==========================================================================*/
Error fatsd_unmount (FSysDescriptor *descriptor)
  {
  FatSD *self = descriptor->self;
  sdcard_eject_card (self->sdcard);
  f_unmount ("0:"); // TODO
  self->mounted = false;
  return 0;
  }

/*============================================================================
 * fatsd_get_capacity
 * ==========================================================================*/
Error fatsd_get_capacity (FSysDescriptor *descriptor, 
         int32_t *total_sectors, int32_t *free_sectors)
  {
  (void)descriptor;
  Error ret = 0;
  FATFS *ff;
  long unsigned int n;
  FRESULT fr = f_getfree ("0:", &n, &ff); // TODO
  if (fr)
    {
    ret = fat_fresult_to_error (fr);
    }
  else
    {
    *free_sectors = (int32_t)(ff->csize * n);
    *total_sectors = (int32_t)(ff->n_fatent - 2) * ff->csize;
    }

  return ret;
  }

/*============================================================================
 * fatsd_get_filedesc_file
 * ==========================================================================*/
FileDesc *fatsd_get_filedesc_file (FSysDescriptor *descriptor, 
      const char *path, int flags, Error *error)
  {
  (void)descriptor;
  FileDesc *ret = NULL;
  // Again, we have to do something about drive number here...
  FATFile *fatfile = fatfile_new (path, flags);
  FileDesc *filedesc = fatfile_get_filedesc (fatfile);
  Error err = filedesc->open (filedesc, flags);
  if (err == 0)
    {
    ret = filedesc;
    *error = 0;
    }
  else
    {
    fatfile_destroy (fatfile); 
    *error = err;
    }
  return ret;
  }

/*============================================================================
 * fatsd_get_filedesc_dir
 * ==========================================================================*/
FileDesc *fatsd_get_filedesc_dir (FSysDescriptor *descriptor, 
      const char *path, int flags, Error *error)
  {
  (void)descriptor;
  FileDesc *ret = NULL;
  // Again, we have to do something about drive number here...
  FATDir *fatdir = fatdir_new (path);
  FileDesc *filedesc = fatdir_get_filedesc (fatdir);
  Error err = filedesc->open (filedesc, flags);
  if (err == 0)
    {
    ret = filedesc;
    *error = 0;
    }
  else
    {
    fatdir_destroy (fatdir); 
    *error = err;
    }
  return ret;
  }

/*============================================================================
 * fatsd_get_filedesc
 * ==========================================================================*/
FileDesc *fatsd_get_filedesc (FSysDescriptor *descriptor,
      const char *path, int flags, Error *error)
  {
#ifdef DEBUG
  DEBUG ("path=%s flags=%04X", path);
#endif

  *error = ENOENT;

  FILINFO finfo;
  // TODO -- how do we handle drives here? Perhaps we need to
  //   reinstate the drive letter? Not a problem when there is ony
  //   one FAT drive

  char _path[PATH_MAX];
  strncpy (_path, path, PATH_MAX);
  fat_remove_trailing_slash (_path);

  // Not that the FAT drive does not support calling f_stat on the root
  //    directory
  if (strcmp (_path, "/") == 0)
    {
    return fatsd_get_filedesc_dir (descriptor, _path, flags, error);
    }

  FRESULT fr = f_stat (_path, &finfo);	
  if (fr == 0)
    {
    if (finfo.fattrib & AM_DIR)
      {
      return fatsd_get_filedesc_dir (descriptor, _path, flags, error);
      }
    }
  return fatsd_get_filedesc_file (descriptor, _path, flags, error);
  }

/*============================================================================
 * fatsd_mkdir
 * ==========================================================================*/
Error fatsd_mkdir (FSysDescriptor *desc, const char *path)
  {
  (void)desc;
  return fat_fresult_to_error (f_mkdir (path));
  }

/*============================================================================
 * fatsd_rename
 * ==========================================================================*/
Error fatsd_rename (FSysDescriptor *desc, const char *source, 
        const char *target)
  {
  (void)desc;
  return fat_fresult_to_error (f_rename (source, target));
  }

/*============================================================================
 * fatsd_rmdir
 * ==========================================================================*/
Error fatsd_rmdir (FSysDescriptor *desc, const char *path)
  {
  (void)desc;
  return fat_fresult_to_error (f_unlink (path));
  }

/*============================================================================
 * fatsd_unlink
 * ==========================================================================*/
Error fatsd_unlink (FSysDescriptor *desc, const char *path)
  {
  (void)desc;
  return fat_fresult_to_error (f_unlink (path));
  }

/*============================================================================
 * fatsd_utime
 * ==========================================================================*/
Error fatsd_utime (FSysDescriptor *desc, 
                 const char *path, const struct utimbuf *times)
  {
  (void)desc;
  DWORD dt;
  if (times->modtime == 0)
    dt = 0;
  else
    {
    int offset = 60 * process_get_utc_offset (process_get_current());
    dt = fat_unix_to_time (times->modtime - offset);
    }
  FILINFO fno = 
    {
    .fdate = (WORD)((dt >> 16) & 0xFFFF),
    .ftime = dt & 0xFFFF 
    }; 
  return fat_fresult_to_error (f_utime (path, &fno));
  }

/*============================================================================
 * fatsd_new
 * ==========================================================================*/
FatSD *fatsd_new (spi_inst_t *spi, int drive_strength, uint gpio_cs,
          uint gpio_miso, uint gpio_mosi, uint gpio_sck, int baud_rate)
  {
  FatSD *self = malloc (sizeof (FatSD));
  self->sdcard = sdcard_new (spi, drive_strength, 
     gpio_cs, gpio_miso, gpio_mosi, gpio_sck, baud_rate);
  FSysDescriptor *descriptor = malloc (sizeof (FSysDescriptor));
  descriptor->mount = fatsd_mount;
  descriptor->unmount = fatsd_unmount;
  descriptor->get_capacity = fatsd_get_capacity;
  descriptor->get_filedesc = fatsd_get_filedesc;
  descriptor->mkdir = fatsd_mkdir;
  descriptor->rename = fatsd_rename;
  descriptor->unlink = fatsd_unlink;
  descriptor->rmdir = fatsd_rmdir;
  descriptor->utime = fatsd_utime;
  descriptor->self = self;
  descriptor->name = strdup ("FAT32 on SD card");
  self->descriptor = descriptor;
  self->mounted = false;
  return self;
  }

/*============================================================================
 * fatsd_destroy
 * ==========================================================================*/
void fatsd_destroy (FatSD *self)
  {
  sdcard_destroy (self->sdcard);
  free (self->descriptor->name);
  free (self->descriptor);
  free (self);
  }

/*============================================================================
 * fatsd_get_descriptor
 * ==========================================================================*/
FSysDescriptor *fatsd_get_descriptor (const FatSD *self)
  {
  return self->descriptor;
  }
#endif

