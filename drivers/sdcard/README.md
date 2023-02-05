# SDCard

This is a simple block device driver for an SD card on the
Rasperry Pi Pico. It is not a filesystem; rather, it is an abstraction
of the underlying hardware that can be used to implement a filesystem
(e.g.m based on FAT32 or LittleFS). The driver can do only three things:

- Initialize an SD card and find its capacity
- Write 512-byte sectors from memory to the card
- Read 512-byte sectors from the card to memory

To use this driver you need an SD card breakout or some very fine
soldering. The SD card must be wired to one of the two SPI interfaces
on the Pico. When the driver is initialized, the user must specify which
interface, and which GPIO pins, are used for the connections. 

## Usage

Basic usage of the library is as follows:

    SDCard *s = sdcard_new (spi0, GPIO_DRIVE_STRENGTH_2MA, 
       17, 16, 19, 18, 1000 * 1000); // Create the SDCard "object"

    sdcard_init (s); // Initialize the driver
    sdcard_insert (s); // initialize the card 

    sdcard_read_sectors (...);
    sdcard_write_sectors (...);

## Limitations

- For simplicity of use, this implementation only supports one SD card.

- CRC checks are always enabled, so cards must support SD

- I've only tested V2 (SDHC) cards. Earlier V1 cards might work, but
  I don't have any to test

- I've made some effort to prevent problems related to concurrency in the
  driver, but these are hard to test for on the Pico. The driver should
  probably be regarded as not thread-safe.

- There is no support for hot-plugging -- once the sdcard is initialized,
  it is expected to stay in its slot. The application can, however, call
  `sdcard_insert()` at any time to reinitialize the card.

- The driver will not attempt to adjust the bus speed according to the
  card type. It certainly won't try to adjust the voltage, because the
  Pico does not support this.

## How it works

### Overview

This driver uses the Pico's built-in SPI support to communicate with
the card. SPI is considered old-fashioned in the SD world, but support
for it in cards remains mandatory at present. The newer SDIO protocol
not only supports a faster clock speed, but can also use multiple 
data lines. However, SDIO is highly proprietary, and it isn't clear to
me whether it would be possible to realize its benefits on a Pico.   

The SPI interface is driven by DMA transfers, which generate
an interrupt on completion. Two DMA channels are used -- one for
transmit and one for receive. 

### SPI

SPI is essentially a 3-wire interface -- a bi-directional pair of data
lines, and a clock. On the Pico end of the SPI connection these
connections are labelled SCK, MISO (master in, slave out), and MOSI
(master out, slave in). The corresponding connections on the SD card
are typically labelled DI (data in) and DO (data out). There's also a
chip select, AKA "slave select", pin on the SD, which the Pico sets low
to enable the SD. Don't blame me for all this "master/slave" stuff --
I didn't choose the terminology.

The chip select line can be any general-purpose GPIO pin, but the
Pico uses specific pin assignments for SPI, and the Pico SDK has
particular APIs for handling it.

Large SPI data transfers, in either direction, are typically done using
DMA. This can be faster than having the controller clock each byte
in and out. The DMA process can trigger an interrupt when it is complete,
so the SD card driver just idles until the interrupt arrives. 

Because my implementation only supports one SD card, I don't have to
work out which interrupt corresponds to which card, which would be a
complication in a more complex implementation.

### SD SPI commands

All SD SPI commands are 6 bytes long -- the command number, a 32-bit
argument, a a one-byte CRC. 

Command numbers are listed starting at CMD0 (a reset), but the value 
actually transmitted has bit 6 sent, that is, the command number plus
64. This process, presumably, limits the total number of available
commands to 63 (but see below).  

The CRC is to protect against the command and argument being incorrectly
sent. Command CRC can be disabled, but it is enabled in this implementation.

The chip select (CS) line must be driven low to enable the SD card, and
should stay low until the command has been transmitted and the complete
response received.  

The response to most commands is an "R1" message consisting of a single
byte. In most cases, success is indicated by the value being zero. However,
the least-significant bit, the idle state flag, is the correct response
to operations that are expected to put the card into the idle state,
such as CMD0 (reset).

As well as the basic one-byte commands, the SD specification also provides
for and extended set of "application" or "app" commands. These are send
by sending CMD55 first, and then the specific command. I presume that this 
procedure exists because command numbers must be 63 or less, and the
whole range is used. In this implementation, the only app command
used is ACMD41, which is part of the initialization sequence for V2
cards. 

### Initializing the card

A detailed explanation of the initialization process -- which is 
rather complicated -- can be found here.

http://rjhcoding.com/avrc-sd-interface-1.php

Part of the complexity comes from the fact that there are a number of
different types of SD card in circulation.

Initialization consists of sending "at least 74" clock pulses with 
the DI line high. This sequence, presumably, allows the card to 
synchronize to the clock. We can achieve this sequence by sending
10 bytes (80 bits) of value 0xFF over SPI. It is this process that
locks the card into using SPI for subsequent operations.

The controller then sends CMD0 (reset) command -- repeatedly if necessary,
until the card responds that it is idle.

Then the controller sends CMD8, to check the type (V1 or V2) of the card.
We can check the supported voltage range here as well, although getting
this wrong will likely have more far-reaching consequences. Note that
we can read a more fine-grained voltage expectation from the OCR later. 

At this point, we can send CMD59 to enable command CRC checks if required,
and send CMD59 to read the OCR. 

Once we know what kind of card we're dealing with, we can trigger the
card's internal initialization process by sending ACMD41. This is an
"app" command, which can be repeated until it responds with a zero, 
indicating that initialization is complete.

Finally, we send CMD58 again to read the OCR again, to figure out whether
the card has high-capacity support. Although we read the OCR earlier,
this information would not have been available -- it is determined
during the ACMD41 initialization process.  

Now we can determine the capacity of the card -- the process for which
is rather mess, as different types of card specify it in different
ways. The basic process is to issue command CMD9 (send card-specific
data, CSD). However, the format of the CSF is awkward, and card-specific.

Regardless of the internal structure of the card, we will send and 
receive data in 512-byte sectors. To tell the card this, we send a
CD16 (set block length) command.  

The whole initialization sequence is usually done with the clock set
to a relatively low speed, usually 400 kbaud. Assuming that everything
up to this point succeeds, we can increase the clock to its 
working frequency.  

### Transfers

To receive data from the card, we first send CMD17 for a single sector or
CMD18 for multiple sectors. Both the commands carry the address to read
on the card. Addresses for high-capacity cards are sectors, but low-capacity
cards use an actual byte offset. 

With multiple-block transfers, the card will send data from the starting 
address until the Pico tells it to stop by sending a CMD12 'stop' command.
For a single-block transfer, only one sector is read, and there is not
need for an explicit stop.

After receiving the acknowledgement to the CMD17 or CMD18, the Pico
must wait until the card sends the start token; this is the single byte
0xFE. This token indicates that actual data follows. The data read is
coordinated by DMA -- the DMA controller is linked to the SPI bus, and
interrupts when a whole sector has been read. This data is stored in
memory.

Then the card sends a two-byte CRC check, which the driver checks against
its own calculated CRC for the block it has received. Then the card sends
a general response token, indicating the success of the previous transfer,
from its point of view.

Finally, the card sends a zero byte, to indicate that it is ready for the
next operation. This ready signal need not follow directly from the 
transfer data -- we need to wait for it.

With a few subtleties, the process for writing a sector is much the
same as for reading.  

## Acknowledgements

I am indebted to Carl J Kugler and Neil Thiessen, whose SD driver 
implementations I referred to extensively.


