/*============================================================================
 *  sys/clocks.h
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

#pragma once

#include <ds3231/ds3231.h>
#include <sys/error.h>

extern DS3231 *ds3231_inst;

#ifdef __cplusplus
extern "C" {
#endif

/* Get the system time, as UTC. This will come from the Pico hardware RTC,
   which will have been synced from the DS3231 if it is fitted. */
time_t clocks_get_time (void);

/* Set the system time and synchronize it to the DS3231, if fitted. 
   Although this function returns an error code, in practice there really
   isn't any error that can be reported, given that the hardware operations
   themselves don't report an error. */
Error clocks_set_time (time_t time);

/* Read the DS3231 and set the Pico's clock from it. No error is reported,
     because this will happen during initialization, when nothing is
     watching. */
void clocks_hardware_to_system (void);

#ifdef __cplusplus
}
#endif



