# Times in BearOS

BearOS manipulates times in Unix epoch format, just to act as a common
unit of exchange between the various timing devices, and to provide a
common C API. 

BearOS has no notion of timezone. It is expected that the hardware clocks
will be set to UTC, and a UTC offset applied to give the local time.
At present the logic for this offset is implemented, but it's not 
actually exposed. 

It is anticpated that a real-time clock based on the DS3231 will be
installed on the I2C bus. This is only read at start-up, and its
time transfered to the Pico's built-in RTC. Thereafter, when the time is
read, it it obtained from the Pico RTC. When the time is set
(e.g, using the `data` command), it is set on both the Pico RTC and the
DS3231.

If no DS3231 is installed, the time will start up at some arbitray 
date and time. This can still be changed using the `date` command, the
time should continue to tick along -- at least until the device is reset.



