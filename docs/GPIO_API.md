# BearOS GPIO API

BearOS applications can do simple GPIO input/output operations on specific
pins without needing any extra libraries. The GPIO interface is exposed
as the character device `p:/gpio`. An application can read and write to
this port to set and read GPIO pins.

* Note: GPIO pin numbers are not, in general, the same as package pins. *
* This API uses GPIO pin numbers, not package pin numbers. * 

To use the interface, write four bytes to the character device, in a
single write() call. It won't hurt to write more characters if the application
likes to send a terminating zero, for example, but only four will be read,
and the rest discarded. 

The four characters consist of a two-character command code, and a two-digit
pin number, that is `CCPP`.

The control codes are:

RE  Reset/initialize pin (set to input mode)
RD  Read pin value (when used as input)
HI  Set pin high (when used as output)
LO  Set pin low (ditto)
PU  Pull up pin (when used as input)
PD  Pull down pin (ditto)
IN  Set pin as input
OU  Set pin as output

Any other code will cause the read call to return -1 and set errno.

These codes are defined in `bearos/gpio.h`.

Regardless of the operation to be performed, the first operation on a 
pin must always be RE, which will have the side-effect of setting that
pin to an input. To set it to an output, issue OU.

To read a pin value, issue the RD command, and then read two bytes. 
These will be the (zero-terminated) strings "0" or "1". The read operation
will never block, and will return nonsense data if the last command was
not an RD.

To set a pin high or low issue HI or LO.

The PU and PD commands set an input pin to pull up or pull down.

It is important to appreciate that the interface expects to receive 
the command and pin number is a single write operation. Operations can't
be grouped into a write, nor is anything buffered.




