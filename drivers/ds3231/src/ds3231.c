/*===========================================================================

  ds3231 -- a driver for the Maxim DS3231 for the Pi Pico

  Copyright (2)2022 Kevin Boone, GPLv3.0 

===========================================================================*/
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#if PICO_ON_DEVICE
#include <hardware/i2c.h>
#include <hardware/gpio.h>
#endif

#include <ds3231/ds3231.h>

/*===========================================================================
  ds3231 opaque structure
===========================================================================*/
struct _DS3231
  {
  int i2c_dev; // 0 or 1
  };

/*===========================================================================
  ds3231_new
===========================================================================*/
DS3231 *ds3231_new (int i2c_dev, int sda_gpio, 
                               int scl_gpio, int i2c_baud)
  {
  DS3231 *self = malloc (sizeof (DS3231));
  memset (self, 0, sizeof (DS3231));
  self->i2c_dev = i2c_dev;
#if PICO_ON_DEVICE
  i2c_init (self->i2c_dev == 0 ? i2c0 : i2c1, (uint)i2c_baud);
  gpio_set_function ((uint)sda_gpio, GPIO_FUNC_I2C);
  gpio_set_function ((uint)scl_gpio, GPIO_FUNC_I2C);
  gpio_pull_up ((uint)sda_gpio);
  gpio_pull_up ((uint)scl_gpio);
#else
  (void)sda_gpio; (void)scl_gpio; (void)i2c_baud;
#endif
  return self;
  }

/*===========================================================================
  ds3231_destroy
===========================================================================*/
void ds3231_destroy (DS3231 *self)
  {
  // There is no cleanup to do, in this application
  free (self);
  }

/*===========================================================================
  dec_to_bcd 
===========================================================================*/
#if PICO_ON_DEVICE
static uint8_t dec_to_bcd (int d)
  {
  return (uint8_t)((d / 10 * 16) + (d % 10));
  }
#endif

/*===========================================================================
  bcd_to_dec 
===========================================================================*/
#if PICO_ON_DEVICE
static int bcd_to_dec (uint8_t d)
  {
  return (d & 0x0F) + ((d & 0xF0) / 16) * 10;
  }
#endif

/*===========================================================================
  ds3231_set
===========================================================================*/
int ds3231_set_datetime (DS3231 *self, int year, int month, int day, 
        int hour, int min, int sec)
  {
  int ret = 0;

  if (year > 100) year -= 2000;

  if (year < 0 || month < 1 || day < 0 || hour < 0 || min < 0 || sec < 0 
      || year > 2099 || month > 12 || day > 31 || hour > 24 || min > 60
      || sec > 60) 
    ret = ERANGE;

  if (ret == 0)
    {
#if PICO_ON_DEVICE
    uint8_t b[8];

    b[0] = 0x00; // Register 0 is seconds; we start there and set the next
		 //   seven registers
    b[1] = dec_to_bcd (sec);
    b[2] = dec_to_bcd (min);
    b[3] = dec_to_bcd (hour);
    b[4] = dec_to_bcd (1); // We don't use the day-of-week register
    b[5] = dec_to_bcd (day);
    b[6] = dec_to_bcd (month);
    b[7] = dec_to_bcd (year);

    i2c_write_blocking (self->i2c_dev == 0 ? i2c0 : i2c1, DS3231_ADDRESS, 
      b, 8, false);
#else
   (void)self; (void)year; (void)month; (void)year; (void)day;
   (void)hour; (void)min; (void)sec;
#endif
    }

  return ret;
  }

/*===========================================================================
  ds3231_get_datetime
===========================================================================*/
void ds3231_get_datetime (const DS3231 *self, int *year, int *month, int *day, 
        int *hour, int *min, int *sec)
  {
#if PICO_ON_DEVICE
  unsigned char b[7];

  uint8_t reg = 0x00;

  i2c_write_blocking (self->i2c_dev == 0 ? i2c0 : i2c1,  DS3231_ADDRESS, 
       &reg, 1,  true);
  i2c_read_blocking (self->i2c_dev == 0 ? i2c0 : i2c1, DS3231_ADDRESS, 
       b, 7, false);
  //printf ("read dev=%d %d %d %d %d %d %d\n", 
  //   self->i2c_dev, b[0], b[1], b[2], b[3], b[4], b[5]); // XXX
  *sec = bcd_to_dec (b[0] & 0x7F);
  *min = bcd_to_dec (b[1] & 0x7F);
  *hour = bcd_to_dec (b[2] & 0x3F);
  // We aren't using the day-of-week register which would be b[3]
  *day = bcd_to_dec (b[4] & 0x3F);
  *month = bcd_to_dec (b[5] & 0x1F);
  *year = bcd_to_dec (b[6]);
#else
  *sec = *min = *hour = *day = *month = *year = 0;
  (void)self;
#endif

  *year += 2000;
/*
  for (int i = 0; i < 7; i++)
    {
    printf ("i=%d v=%02x\n", i, buff[i]);
    }
*/
  }


/*===========================================================================
  ds3231_get_temp
===========================================================================*/
int ds3231_get_temp (const DS3231 *self)
  {
#if PICO_ON_DEVICE
  unsigned char b[2];

  uint8_t reg = 0x11; // temp is registers 0x11 and 0x12

  i2c_write_blocking (self->i2c_dev == 0 ? i2c0 : i2c1,  DS3231_ADDRESS, 
       &reg, 1,  true);
  i2c_read_blocking (self->i2c_dev == 0 ? i2c0 : i2c1, DS3231_ADDRESS, 
       b, 2, false);
  // Reg 0x11 is temp in celcius; the top two bits of reg 0x12
  //   are the fractional degrees. So, for example, "01" is 0.25 degrees.
  // We multiply up all these values by 1000, to give an integer number of
  //   "millicelcius"

  int t = b[0] * 1000 + (b[1] >> 6) * 250;

#else
  (void)self;
  int t = 0;
#endif

  return t;
  }


