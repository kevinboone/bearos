# BearOS for Pico

Kevin Boone, December 2022

* Work in progress. Collaboration is welcome. As a resulf of various * 
* conversations I have had with other retrocomputing enthusiasts, * 
* I am publishing this before it is remotely complete * 

* Please note that, at present, this system has to be operated using. *
* a terminal. I have a rudimentary driver for the Waveshare 480x320 LCD *
* panel, but as yet no keyboard support. Ultimately I want to make a completely *
* self-contained retro-style computer with its own operating system, but
* this is some way off. *

* For sample programs for BearOS, see my other repoistory 'bearos\_utils'. *

## What is this?

BearOS ("Basic Embedded All-purpose Retro Operating System") is a
proof-of-concept, single-tasking, single-user operating system for the
Raspberry Pi Pico. Its capabilities are broadly similar to those of MSDOS or
CP/M. At present it supports only command-line/console operation, driven by a
serial terminal. The command shell and many of the built-in utilities are
Linux-like in operation, but there is no similarity to Linux internally.

BearOS is a user-level operating system, and should be contrasted with
developer-level platforms like FreeRTOS. FreeRTOS provides OS-like task
management to components of an application. BearOS is designed for a human
operator to select and run programs (one at a time).

BearOS uses an SD card as its primary storage. Although it will boot without
one, it will be of little use. To be fair, it's of little use anyway; without
an SD card it is of less use even than that.  It is very easy to connect an SD
card (see wiring instructions below).  

The card must be pre-formatted as a
single partition VFAT filesystem.  SD cards are often supplied formatted this
way. The directory `/bin' is, by default, used to store executable programs.
Some utiliies assume that the directories `/home` and `/tmp' exist on the card.

BearOS also supports a real-time clock based on the DS3231. Again, it will boot
without one, but anything that even approximates a 'real' end-user operating
system needs a robust clock. A DS3231 module is very easy to connect to the
Pico -- again, see the wiring section below.

## Why? 

Good question.

BearOS is an attempt to show that it's possible to make a usable,
general-purpose computer from a Pi Pico, which only costs a few pounds.
However, with only 2Mb flash and 260kB RAM, the Pico does not have the
capabilities to run a modern operating system efficiently. In particular, it
doesn't have enough RAM for serious user-level multitasking.

And yet, we were able to do real computing with CP/M and MSDOS 40 years ago,
and these platforms seem primitive by modern standards.  It just requires a
different developer mind-set -- one that focuses on efficiency and parsimony. 

BearOS is designed to be an instant-on, instant-off system. Booting up is
expected to be instantaneous (in human terms), and shutting down amounts to
removing the power source.  

Essentially, BearOS is a self-educational project. Whether it has any
application beyond this, or holds any interest other than for retro-computing
enthusiasts, I don't know.

## Design goals

- BearOS will eventually be self-contained, with its own inexpensive LCD
display and keyboard/
- BearOS is instant-on, instant-off. There is no specific start-up or shut-down
  procedure
- Programs for BearOS can be cross-compiled on a Linux system using
  widely-available tools. Eventually it will be possible to develop on BearOS
itself
- BearOS exposes hardware as character devices. The driver for the I2C LCD
  module shows how this works in practice
- The filesystem and block device logic are separate in the kernel. It will be
  relatively easy to add different block devices in future. 

## Design non-goals

- BearOS will never be an alternative to a modern operating system. In
  particular, it is not intended to be a 'single-tasking Linux'. Similarities
to Linux are purely for ease of memory.
- BearOS will never support any multi-tasking operation. Keeping the whole system single-tasking makes the kernel much, much faster.
- BearOS will never implement any kind of security. It is a single-user system, and that use has all access to everything.

These design choices are purely to make BearOS fast, and so that the entire
kernel and shell fit into the Pico's 2Mb flash.

## What currently works

- There are basic, built-in utilities for manipulating files on the SD card.
  `cp`, `mv`, `cat`, `rm`, `mkdir`, etc. Despite the Linux-like names -- which
are just for convenience -- this isn't remotely a Linux platform. 

- The built-in time management works, if an RTC clock device is installed. So
  the `date` utility can get and set the date and time, and correct timestamps
are written when files are updated.

- The command-line shell is basically functional. It supports a line editor,
  environment manipulation, wildcard expansion, redirection, and pipes.
Script-like operation is also possible, with files being read and executed
line-by-line.

- Programs can be loaded from the SD card and executed. There is a concept of
  search path, using the 'PATH' environment variable. Building programs for
BearOS is neither trivial nor well-documented, but there are some examples.
Please see the separate repository `bearos_utils` for sample utilities.

- There is a (just about) working text editor, called `bute`. It's _very_
  fussy, however, about the set-up of the terminal.

- There is a port of the GNU `bc` utility, which can do basic floating-point
  math operations, and can be extended using libraries

- There is a port of `frotz`, which can run _some_ interactive fiction games.
  The limitation here is memory. A better way to run Zork, et., al, is to run
the CP/M versions under an emulator. Because...

- There is a CP/M emulator, that can run Wordstar, etc. It's very slow -- about
  the speed of real CP/M in the 1980s. Again, CP/M applications are all fussy
about the terminal set-up (as they always were).

- There's a bunch of other, slightly useful utilities for things like dumping
  memory or files in hex, paging through files, etc. This group of utilities
will expand in due course.

- There is a character device interface to the GPIO system. Any program that
  can read and write a file should be able to set and read GPIO pins. See
`docs/GPIO_API.md` for details. 

## Building the BearOS kernel

BearOS is designed to be built using the Pi Pico SDK, which uses CMake as the
build manager (for better or worse). This isn't the place to explain how to
install and set up the Pico SDK, but it is well-documented. To build for the
Pico:

    mkdir build_arm
    cd build_arm
    PICO_SDK_PATH=/path/to/sdk cmake ..
    make

It's possible to build a Linux version that tests the basic functionality
of the shell and kernel: see `docs/BUILD\_LINUX.md` for more information.


## Developing for, or porting to, BearOS

C applications can be built for BearOS using the same compiler
(`arm-none-eabi-gcc`) and related tools that are used by the Pico C SDK.  So if
you have the C SDK, you already have the tools to build for BearOS. I've only
tried this on Linux -- I don't know what would be involved in using any other
platform.

It is my intention eventually to support the full C standard library, although
some features will never work. For example, all API functions concerned with
users or process control won't ever work, as this is a single-user,
single-tasking platform. However, fundamental disk I/O operations do work
(`open`, `fopen`, `read`...); there is a concept of "standard in" and "standard
out"; dynamic memory allocation (`malloc`, etc) works, subject to the
constraint that there is very little RAM. 

BearOS has no concept of libraries or shared objects -- the application must
include all the code it uses, apart from that which is included in the BearOS
kernel. Because there is so little RAM, but comparatively plentiful flash, the
kernel exposes higher-level operations that would not normally be provided by a
real operating system.  These include, for example, a function to read a line
of text with line editing. Of course, programs that use these features will be
specific to the BearOS platform. 

To build for BearOS you'll need access to the `api/` directory of the kernel
source, but no other kernel code is required. The process of compilation and
linking is a bit fiddly, to say the least. The separate `bearos_utils` package
contains BearOS ports of a few common utilities. These demonstrate how the
process works.

For more information on building programs for BearOS, see
`docs/PORTING_TO_BEAROS.md`.

## Wiring the peripherals

BearOS requires at least an SD card to be even remotely functional.  A
real-time clock chip is also strongly recommended.

### SD card

BearOS requires an SD card, to be of any use at all. It will boot without
one, but few of its features will be available. 

The wiring of the SD card is as follows.

    Pico pin      Pico GPIO    SD Card
    --------      ---------    -------

    29, GPIO 22   22           CS
    13, GND                    GND
    14, SPI1 SCK  10           CLK
    15, SPI1 TX   11           DI
    16, SPI1 RX   12           DO
    36, 3.3V                   VCC

This wiring uses SPI bus 1, and arbitrary GPIO 22 for chip select.  You can
change the SPI bus and the pin assignments by editing the "SD card" section in
`config.h`

### Real time clock

BearOS supports a real-time clock based on the DS3231. It is assumed to
be connected to the I2C0 bus. 

    Pico pin           Pico GPIO    DS3231
    --------           ---------    --------
    Pin 26, SDA1       20           Data / SDA
    Pin 27, SCL1       21           Clock / SCL
    Pin 36, 3.3V out                Vcc / +
    Pin 38, GND                     Gnd / -

You can set the time and date using the 'date' command at the shell prompt. But
see the section on timezones below.

BearOS will work without the real-time clock, but files created or updated will
show useless datestamps and, of course, the 'date' command won't show the right
time. However, if the time is set, it should retain the correct time as long as
power is applied, even without the RTC.

### I2C LCD

BearOS supports an HD4480 I2C LCD display connected to I2C0 (same wiring as
above).  The I2C address can be changed in config.h. The device appears as a
character device `p:lcd`. It auto-scrolls, and responds to some ASCII control
characters: backspace, form feed, line feed, carriage return.

## Shell and command line

The interactive shell prompt supports limited line editing -- use left/right,
ctrl+left/right, home/end keys to move the cursor around.  A small history of
previous command lines (default 10) is retained -- use the up/down arrow keys
to retrieve them.

The double-quote character is used to enclose command-line arguments
into a single token. So

    "file with spaces.txt"

is a single argument. Without the quotes, 

    file with spaces.txt

would be three separate arguments. The double-quotes also prevent
environment variables being expanded.

As in Linux, environment variables are introduced using the dollar sign.  The
name of the variale extends from the dollar sign to the next alphanumeric
character. To make this clearer, you can also put the name in brackets, like
this:

    ${HOME}/mydir

A single command line can contain any number of variable expansions.

The shell wildcard characters ? and * match one or more character in a
filename, as in Linux shells. Again as in Linux, a wildcard that does not match
is left in its argument unchanged. The same is true if these characters are
enclosed in double-quotes.	

There is limited shell scripting support. The command `source {filename}` reads
a file one line at a time and processes it, as if the lines had been entered on
the command line. Any changes made to the environment in the script are
retained in the shell.  

For more information about the shell, see `docs/SHELL.md`.

## Filenames in BearOS

BearOS uses Unix-style directory separators (/), but it also uses drive letters
to identify devices, as CP/M and MSDOS do. Operating this way simplifies and
speeds up a lot of file operations in the Kernel, but it has the potential to
make porting programs to BearOS difficult.

Since BearOS currently supports only one storage device -- an SD card as drive
'A:' -- this decision has no implications, because the drive letters can
generally be ignored. However, I'm keeping this policy under review, because I
can see how it might cause problems in the future.

Note that, although BearOS currently supports one physical storage device,
virtual drives are used for other purposes. So the drive letters aren't
completely invisible.

## Program search path

BearOS searches for executables in locations specified in the environment
variable `$PATH`. Like Windows, and not like Linux, path components are
separated by semicolon, not colon, characters. This is because BearOS uses
drive letters, so the colon character could be part of a directory name. 

Unlike Linux, the search path includes the current directory by default: you
can run 'myprog' in the current directory, rather that './myprog'.  This may be
changed, if it turns out to be a nuisance. 

## Timezone

At present, BearOS does not support timezones. It isn't clear whether a
conventional timezone database will fit in the Pico flash. It is expected that,
if a real-time clock is in use, its time is set to UTC. However, it is possible
to specify a system offset from UTC by setting the environment variable
`UTC_OFFSET` to a number of minutes _ahead_ of UTC. So in France, for example,
`UTC_OFFSET` should be 60 in winter and 120 in summer. With this setting in
place, utilities like `date` will work in the local time. In addition,
timestamps written to files on the SD card will be in local time, which is
conventional with FAT filesystems. 

## Problems

The BearOS core and all the utilities that current exist assume that the
display device is a fixed-sized terminal with the following characterstics:

- Line wrapping is enabled
- The 'enter' key generates a carriage return (13) character
- The terminal is exactly 80 characters wide, and at least 24 rows tall.
- Sending a backspace (8) character is non-destructive

Apart from the line-wrapping, these are the default settings for Minicom.
To enable line wrapping in Minicom, enter Ctrl+A,W. 

The BearOS kernel source is badly organized, and I've made a number of
development decisions to provide short-term solutions that I know will require
serious re-working later.

The reliance on a serial terminal makes porting programs to BearOS a bigger
challenge than it ought to be. Eventually, I want BearOS to use its own display
device and keyboard. However, in keeping with the design philosophy, these will
have to be dirt cheap.

The sample programs for BearOS all use the ARM 'Newlib' standard C library,
supported by stubs provided by the BearOS kernel API.  Even the 'nano' version
of Newlib is _not_ memory-efficient. Even 'Hello, World' requires about 30kB of
RAM. This seems a tiny amount by modern standards, but bear in mind that the
Pico only provides about 240kB in total. It's often more efficient to run a
CP/M utility under an emulator, than a native BearOS utility. In the longer
term, I need to investigate alternatives to Newlib. Even building Newlib from
source, with the compiler set to optimize for size, might be an improvement.

There are no keyboard interrupts. That is, there is no easy way to
interrupt a running program or command using ctrl+c or something of that
nature. The single-tasking nature of BearOS makes handling interrupts
difficult. Of course, no harm should be done simply be switching the
Pico off and on to interrupt it.

## Source contents

`api` -- source files and headers that are required for building programs for
BearOS. 

`chardev` -- drivers for character devices (currently there is a GPIO device, and I2C LCD device and, of course, the console).

`compat` -- functions that widely exist in C, but are not preset into the Pico
SDK, or work differently on the Pico.

`devmgr` -- the device manager. Manages device mounts and matches pathnames to devices.

`docs` -- documentation

`drivers` -- low-level hardware support

`fs` -- filesystem implementation. At present, only FAT32 is supported.

`klib` -- general utility modules

`shell` -- the BearOS shell parser and built-in commands

`sys` -- the system call interface to the kernel

`syslog` -- logging functions

`term` -- functions for handling terminal operations, including the line
editor.

