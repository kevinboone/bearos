# Graphics device API

At present, the only graphics device supported is the Waveshare 480x320
LCD panel, which uses an ST7788 controller. This device has a socket
for the Pico, so there is no flexibility in how the controller lines are
wired to GPIO pins.

The low-level API is crude, and might change in future. 

It's important to understand that the LCD panel has twice as much RAM
as the Pico has. Moreover, the only efficient way to use it is to transfer
large-ish blocks of data from RAM. Writing a single pixel amounts to 
filling a reqion of size 1x1, so the overhead in writing one pixel is the
same as writing a block of any size. So there's really no way to implement
a Linux-like framebuffer API, where pixels are randomly addressable.

The API is based on `devctl` calls, with the following function codes
(defined in `devctl.h`).

`DC_GFX_SET_REGION`

arg1 : not used
arg2 : pointer to a DevCtlGfxRegion structure.

This call sets the region of the display that will be written by the following
operation. The region can be 1x1 pixel. If it's larger than the panel size,
weird things will happen. The driver does not check for this -- it just 
introduces too much overhead.

`DC_GFX_FILL`

arg1 : not used
arg2: pointer to a DevCtlGfxColour union.

Fill the region defined by `DC_GFX_SET_REGION` with the colour set in
the union. A union is used because, in future, other devices might support
other colour formats. At present, however, the Waveshare LCD supports only
RGB565 format, which uses 16 bits per pixel.

`DC_GFX_WRITE`

arg1 : number of _bytes_ to write to the display
arg2 : pointer to the data to write

The data is written to the region identified by the previous `DC_GFX_FILL`
call. The data should be in platform endianness (that is, least-significant
byte first). 

*Note:* the amount of data to write is specified in bytes, not pixels.

`DC_GET_GEN_FLAGS`

arg1 : not used
arg2 : pointer to a 32-bit integer.

This call sets the flag `DC_FLAG_ISGFX`.

`DC_GFX_GET_PROPS`

arg1 : not used
arg2 : pointer to a `DevCtlGfxProps` structure.

This call fills in the `DevCtlGfxProps` with basic information about 
the size and colour format of the display.

