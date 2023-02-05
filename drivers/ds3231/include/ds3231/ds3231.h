/*===========================================================================

  ds3231 -- a driver for the Maxim DS3231 for the Pi Pico

  Usage:

  DS3231 *ds3231 = ds3231_new (I2C_DEV, SDA, SCL, I2C_BAUD);

  ds3231_set_datetime (ds3231, year, month, day, hour, min, sec);
  ds3231_get_datetime (ds3231, &year, &month, &day, &hour, &min, &sec);
  ...

  ds3231_destroy (ds3231);

  Copyright (2)2022 Kevin Boone, GPLv3.0 

===========================================================================*/

#pragma once

// The I2C address of the clock IC. In reality, this is not configurable 
//   without additional hardware -- 0x68 is factory-set.

#define DS3231_ADDRESS 0x68

struct _DS3231;
typedef struct _DS3231 DS3231;

#ifdef __cplusplus
extern "C" { 
#endif

/** Initialize a DS3231 object with the specified I2C device (0 or 1),
      the specified SDA and SCL GPIOs, and I2C baud rate. The DS3231 is
      advertised as supporting 400,000 baud, but this requires careful
      wiring. Note that the SDA and SCL numbers are GPIO numbers and _not_
      package pin numbers. */

extern DS3231 *ds3231_new (int i2c_dev, int sda_gpio, 
         int scl_gpio, int i2c_baud);

/** Clean up the small amount of memory used by the DS3231 object. */
extern void ds3231_destroy (DS3231 *self);

/** Set the date and time. Argument ranges are --
     year -- 2000 to 2099
     month -- 1 to 12
     day -- 1 to 31
     hour -- 0 to 23
     sec -- 0 to 59. 
   Return value is if the operation succeeds, or ERANGE is
     one of the values is out of range. */
extern int ds3231_set_datetime (DS3231 *self, int year, int month, int day, 
         int hour, int min, int sec);

/** Get the date and time. This operation always succeeds, although the
      results might be meaningless if, for example, no DS3231 is 
      connected. */
extern void ds3231_get_datetime (const DS3231 *self, int *year, 
        int *month, int *day, int *hour, int *min, int *sec);

/** Get the temperature in 'millicelcius'. I have adopted this odd unit
      so that this driver can be used in applications where there is no
      floating-point math support. */
extern int ds3231_get_temp (const DS3231 *self);

#ifdef __cplusplus
}
#endif


