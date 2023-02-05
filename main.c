/*===========================================================================
 
BearOS

main.c

This file contains the start function for the BearOS kernel.

It initializes the hardware and filesystem drivers, then runs the shell.

Copyright (c)2022 Kevin Boone, GPL3
 
===========================================================================*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pico/stdlib.h>
#if PICO_ON_DEVICE
#include <hardware/rtc.h>
#include <fatfs_sdcard/fatsd.h>
#else
#include <fatfs_loopback/fatloopback.h>
#endif
#include <waveshare_lcd/waveshare_lcd.h>
#include <ds3231/ds3231.h>
#include <devmgr/devmgr.h>
#include <sys/fsmanager.h>
#include <devfs/devfs.h>
#include <syslog/syslog.h>
#include <sys/diskinfo.h>
#include <sys/clocks.h>
#include <chardev/picoconsoledev.h>
#include <chardev/i2clcddev.h>
#include <chardev/gpiodev.h>
#include <chardev/gfxcondev.h>
#include <chardev/wslcddev.h>
#include <sys/error.h>
#include <errno.h>
#include <term/term.h>
#include <shell/shell.h>
#include <sys/process.h>
#include <sys/syscalls.h>
#include "config.h" 

/*=============================================================================
 * main
=============================================================================*/
int main() 
  {
  stdio_init_all();

#if PICO_ON_DEVICE
  rtc_init();

   // Start clock at _some_ time, in case sync with the DS3231 
   //   fails. 
   datetime_t t = {
            .year  = 2022,
            .month = 12,
            .day   = 23,
            .dotw  = 1,
            .hour  = 11,
            .min   = 10,
            .sec   = 00
    };
  rtc_set_datetime(&t);
#endif

  fsmanager_init();
  devmgr_init();

  PicoConsoleDev *pcd = picoconsoledev_new ("con");
#ifdef I2C_LCD_CONNECTED
  I2CLCDDev *lcd = i2clcddev_new ("lcd");
#endif
  WSLCDDev *wslcddev = wslcddev_new ("gfx");
  GfxConDev *gfxcondev = gfxcondev_new ("gfxcon", 
    wslcddev_get_desc (wslcddev)); 

#ifdef I2C_LCD_CONNECTED
#if PICO_ON_DEVICE
  i2clcddev_init (lcd, LCD_WIDTH, LCD_HEIGHT, I2C_LCD_ADDRESS, 
     I2C_LCD_I2C,
     I2C_LCD_SDA_PIN, I2C_LCD_SCL_PIN, I2C_BAUD, 
     SCROLLBACK_PAGES);
#else
  i2clcddev_init (lcd, LCD_WIDTH, LCD_HEIGHT);
#endif
#endif

  DS3231 *ds3231 = ds3231_new (DS3231_I2C, DS3231_SDA_PIN, 
     DS3231_SCL_PIN, DS3231_I2C_BAUD);
  ds3231_inst = ds3231;
  clocks_hardware_to_system();

/*
  while (true)
    {
  clocks_hardware_to_system();
  sleep_ms (1000);
    }
*/

  GPIODev *gpio = gpiodev_new ("gpio");

  devmgr_register (picoconsoledev_get_desc (pcd));
#ifdef I2C_LCD_CONNECTED
  devmgr_register (i2clcddev_get_desc (lcd));
#endif
  devmgr_register (gpiodev_get_desc (gpio));
  devmgr_register (wslcddev_get_desc (wslcddev));
  devmgr_register (gfxcondev_get_desc (gfxcondev)); // XXX

#if PICO_ON_DEVICE
  FatSD *fatsd = fatsd_new (SD_SPI, SD_DRIVE_STRENGTH, 
     SD_CHIP_SELECT, SD_MISO, SD_MOSI, SD_SCK, SD_BAUD);
#else
  FatLoopback *fatloopback = fatloopback_new();
#endif


  DevFS *devfs = devfs_new();

#if PICO_ON_DEVICE
  fsmanager_mount (0, fatsd_get_descriptor (fatsd));
#else
  fsmanager_mount (0, fatloopback_get_descriptor (fatloopback));
#endif
  
  fsmanager_mount (FSMANAGER_MAX_MOUNTS - 1, devfs_get_descriptor (devfs));
  
  // It seems to be necessary to do this after the FS mount stuff. Perhaps
  //   something weird happens to the SPI bus? 

#if PICO_ON_DEVICE
  wslcddev_init (wslcddev, WSLCD_SPI, WSLCD_CS, WSLCD_MISO, 
    WSLCD_MOSI, WSLCD_SCK, WSLCD_RST, WSLCD_DC, 
    WSLCD_BL, WSLCD_BAUD, WSLCD_SCAN_DIR);
#else
  wslcddev_init (wslcddev);
#endif

  gfxcondev_adjust_to_hardware (gfxcondev);
  gfxcondev_full_reset (gfxcondev); // SLOW
  
  Process *p = process_new();
  process_setenv (p, "UTC_OFFSET", "0"); 
  process_setenv (p, "PATH", "A:/exec;A:/bin");
  process_setenv (p, "HOME", "A:/home");
  process_setenv (p, "TMP", "A:/tmp");
  process_open_file (p, 0, "p:con", O_RDONLY);
  process_open_file (p, 1, "p:con", O_WRONLY);
  process_open_file (p, 2, "p:con", O_WRONLY);
  process_run (p, shell_process, 0, NULL);
  process_destroy (p);

  //shell_run (termdev_get_descriptor(termdev), 
  //      termdev_get_descriptor(termdev));

#if PICO_ON_DEVICE
  fatsd_destroy (fatsd);
#else
  fatloopback_destroy (fatloopback);
#endif

  devfs_destroy (devfs);
  devmgr_deinit();
  picoconsoledev_destroy (pcd);
#ifdef I2C_LCD_CONNECTED
  i2clcddev_destroy (lcd);
#endif
  gpiodev_destroy (gpio);
  wslcddev_destroy (wslcddev);
  gfxcondev_destroy (gfxcondev);
  ds3231_destroy (ds3231);

  exit (0);
  }


