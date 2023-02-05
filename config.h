/**
 
config.h -- configuration parameters for BearOS on Pi Pico 

These sample settings are for hardware provided by the WaveShare 3.5-inch
LCD/SD module, which can accomodate a piggybacked WaveShare DS3231 real-time
clock module. The GPIO pin assigments are correct for this combination, and
need not be changed.  If you're assembling your own hardware you are, of
course, free to choose the pins you want.

*** IMPORTANT ***

The Pico can assign different GPIO pins to the same bus. For example, I2C0
can be on pins 0/1, 4/5, 8/9, 16/17, etc. This can be helpful for wiring,
but real caution is needed. It is not possible to use the same I2C (or SPI,
or anything else) bus with _different_ pin groups. For example, you can't 
connect an I2C device to GPIO 0/1, and a different device to 4/5 -- whichever
device is initialized last will control the pin assignments. 

Similarly, you can't set different bus properties to different devices on the
same bus. The Waveshare LCD, for example, has the SD card and the LCD panel
both on SPI1. Both must be set to use the same pins, use the same drive
strength, and the same speed. 

If using the Waveshare RTC module, bear in mind that (at least) two versions
are in circulation. One is wired to I2C1, using pins 6/7, and another on 
I2C0 using 20/21. Both are compatible with the Waveshare LCD, but there's
no way that I can determine to tell them apart, except by testing. 

Copyright (c)2023 Kevin Boone, GPL v3.0 
*/

/*================== SD card settings =================================== */

#define SD_SPI             spi1
#define SD_DRIVE_STRENGTH  GPIO_DRIVE_STRENGTH_2MA
#define SD_CHIP_SELECT     22 
#define SD_MISO            12 
#define SD_MOSI            11
#define SD_SCK             10 
#define SD_BAUD            (20000 * 1000)

/*============== ILI9488 LCD panel driver settings ====================== */

#define WSLCD_SPI         spi1
#define WSLCD_CS          9
#define WSLCD_MISO        12
#define WSLCD_MOSI        11
#define WSLCD_SCK         10
#define WSLCD_RST         15
#define WSLCD_DC          8
#define WSLCD_BL          13
#define WSLCD_BAUD        (20000 * 1000)
#define WSLCD_SCAN_DIR    WSLCD_SCAN_NORMAL

/* ====================== General I2C settings  ==================== */
// I2C baud rate. 100k seems a safe value. With short, tidy connections,
//   this could usefully be set higher.
#define I2C_BAUD 100000

/* ====================== I2C LCD settings  ==================== */

// Define this to enable the I2C LCD driver, based on an HD4480 device.  It
//   won't hurt to enable it even if one isn't connected, but you'll get a
//   character device that doesn't do anything
//#define I2C_LCD_CONNECTED 

// Size of the scrollback buffer, in screen pages. That is, with a 
//   four-line display, a page is four lines. There's no problem with
//   increasing this, except memory.
#define SCROLLBACK_PAGES 10

// The I2C address of the I2C LCD device. Common values are 0x27 and 0x3F. 
// Some devices can have their I2C addresses set using jumpers.
#define I2C_LCD_ADDRESS 0x27
#define I2C_LCD_SDA_PIN 20
#define I2C_LCD_SCL_PIN 21
// I2C bus -- 1 or 0
#define I2C_LCD_I2C 0

// The size of the LCD. Common sizes are 16x2, 20x4 and 40x2
#define LCD_WIDTH  20 
#define LCD_HEIGHT 4

/* ==================== DS3231 RTC settings (uses I2C) ================== */

// I2C bus -- 0 or 1
#define DS3231_I2C 0
#define DS3231_SDA_PIN 20 
#define DS3231_SCL_PIN 21 
#define DS3231_I2C_BAUD I2C_BAUD 


