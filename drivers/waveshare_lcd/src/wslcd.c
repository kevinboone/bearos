#include <stdint.h> 
#include <string.h> 
#include <malloc.h> 
#include <stdio.h> 
#if PICO_ON_DEVICE
#include <hardware/spi.h> 
#endif
#include <hardware/gpio.h> 
#include <waveshare_lcd/waveshare_lcd.h> 

// ID codes for the 2.8" and 3.5" displays, from the Waveshare docs
#define LCD_2_8	0x52
#define LCD_3_5	0x00

#define LCD_X_MAXPIXEL  320 
#define LCD_Y_MAXPIXEL  480 

// We need to keep this potential X offset for now, because I'm not sure the
//   display actually extends all the way to the left margin :/
#define LCD_X 0
#define LCD_Y 0

#define LCD_3_5_WIDTH  (LCD_X_MAXPIXEL - 2 * LCD_X)
#define LCD_3_5_HEIGHT  LCD_Y_MAXPIXEL 

#define LCD_2_8_WIDTH  	240  //LCD width
#define LCD_2_8_HEIGHT  320

// Opaque structure

struct _WSLCD
  {
#if PICO_ON_DEVICE
  spi_inst_t *spi;
#endif
  uint gpio_cs;
  uint gpio_miso;
  uint gpio_mosi;
  uint gpio_sck;
  uint gpio_rst;
  uint gpio_dc;
  uint gpio_bl;
  int baud_rate;
  int height;
  int width;
  int used_height;
  int used_width;
  WSLCDScanDir scan_dir;
  uint8_t id;
  uint16_t scroll_pos;
  };

static void _wslcd_set_scroll_area (WSLCD *self, uint16_t tfa, 
         uint16_t vsa, uint16_t bfa); // FWD
static void wslcd_set_scroll_area (WSLCD *self); // FWD

#if PICO_ON_DEVICE

/*============================================================================
 wslcd_gpio_init
 Set the direction and inital state of all the GPIO pins. Note: we need to
   do this even before calling _reset(), since _reset() requires the RST pin
   to be set up
 *===========================================================================*/
static void wslcd_gpio_init (const WSLCD *self)
  {
  gpio_init (self->gpio_rst);
  gpio_init (self->gpio_dc);
  gpio_init (self->gpio_bl);
  gpio_init (self->gpio_cs);
  gpio_set_dir (self->gpio_rst, GPIO_OUT);
  gpio_set_dir  (self->gpio_dc, GPIO_OUT);
  gpio_set_dir  (self->gpio_bl, GPIO_OUT);
  gpio_set_dir  (self->gpio_cs, GPIO_OUT);
  //  DEV_GPIO_Mode(TP_CS_PIN,GPIO_OUT);
  //  DEV_GPIO_Mode(TP_IRQ_PIN,GPIO_IN);
  //  gpio_set_pulls(TP_IRQ_PIN,true,false);
  //  gpio_put(TP_CS_PIN, 1);
  gpio_put (self->gpio_cs, 1);
  gpio_put (self->gpio_bl, 1);
  //  gpio_put(SD_CS_PIN, 1);
  }

/*============================================================================
 wslcd_write_byte
 *===========================================================================*/
static uint8_t wslcd_write_byte (const WSLCD *self, uint8_t value)
  {   
  uint8_t rx;
  spi_write_read_blocking (self->spi, &value, &rx, 1);
  return rx;
  }

/*============================================================================
  swap16 
  Swaps the byte order of x, in place
 ===========================================================================*/
static inline void swap16 (uint16_t *x)
  {
  uint8_t *t = (uint8_t *)x;
  uint8_t c = t[0];
  t[0] = t[1];
  t[1] = c;
  } 

/*============================================================================
  spit_write_blocking2
 ===========================================================================*/
static inline void spi_write_blocking2 (spi_inst_t *spi, const uint8_t *src) 
  {
  for (size_t i = 0; i < 2; ++i) 
    {
    while (!spi_is_writable(spi))
      tight_loop_contents();
    spi_get_hw(spi)->dr = (uint32_t)src[i];
    }

  while (spi_is_readable(spi))
    (void)spi_get_hw(spi)->dr;
  while (spi_get_hw(spi)->sr & SPI_SSPSR_BSY_BITS)
    tight_loop_contents();
  while (spi_is_readable(spi))
    (void)spi_get_hw(spi)->dr;

  spi_get_hw(spi)->icr = SPI_SSPICR_RORIC_BITS;
  }

/*============================================================================
  wslcd_write_word_be
  Writes w in big-endian order to the SPI bus
 ===========================================================================*/
static inline void wslcd_write_word_be (const WSLCD *self, uint16_t w)
  {
  swap16 (&w);
  //uint8_t rx[2];
  //spi_write_read_blocking (self->spi, (uint8_t *)&w, rx, 2);
  spi_write_blocking2 (self->spi, (uint8_t *)&w);
  }

/*============================================================================
  wslcd_reset 
  Do a hardware reset by asserting the reset line. TODO: do we need to delay
    for as long as this?
 *===========================================================================*/
static void wslcd_reset (const WSLCD *self)
  {
  gpio_put  (self->gpio_rst, 1);
  sleep_ms (200);
  gpio_put (self->gpio_rst, 0);
  sleep_ms (200);
  gpio_put (self->gpio_rst, 1);
  sleep_ms (200);
  }

/*============================================================================
  wslcd_write_reg
  Send a register number
 *===========================================================================*/
static void wslcd_write_reg (const WSLCD *self, uint8_t reg)
  {
  gpio_put (self->gpio_dc, 0);
  gpio_put (self->gpio_cs, 0);
  wslcd_write_byte (self, reg);
  gpio_put (self->gpio_cs, 1);
  }

/*============================================================================
  wslcd_write_data
 *===========================================================================*/
void wslcd_write_data (const WSLCD *self, uint16_t data)
  {
  if (self->id == LCD_2_8)
    {
    // Not tested!
    gpio_put (self->gpio_dc, 1);
    gpio_put (self->gpio_cs, 0);
    wslcd_write_byte (self, (uint8_t)data);
    gpio_put (self->gpio_cs,1);
    }
  else
    {
    gpio_put (self->gpio_dc, 1);
    gpio_put (self->gpio_cs, 0);
    //wslcd_write_byte (self, (uint8_t) (data >> 8));
    wslcd_write_byte (self, (uint8_t) (0));
    wslcd_write_byte (self, (uint8_t) (data & 0xFF));
    gpio_put (self->gpio_cs, 1);
    }
  }

/*============================================================================
  LCD_Read_Id 
  Copied from the original Waveshare example, but I'm far from convinced that
    it returns a meaninfgul value.
 ===========================================================================*/
uint8_t LCD_Read_Id (WSLCD *self)
  {
  uint8_t reg = 0xDC;
  uint8_t tx_val = 0x00;
  uint8_t rx_val;
  gpio_put (self->gpio_cs, 0);
  gpio_put (self->gpio_dc, 0);
  wslcd_write_byte (self, reg);
  spi_write_read_blocking (self->spi, &tx_val, &rx_val, 1);
  gpio_put (self->gpio_cs, 1);
  return rx_val;
  }

/*============================================================================
  wslcd_initreg 
 ===========================================================================*/
static void wslcd_initreg (WSLCD *self)
  {
  self->id = LCD_Read_Id (self);
  if (self->id == LCD_2_8)  
    {
    wslcd_write_reg (self, 0x11);
    sleep_ms(100);
    wslcd_write_reg (self, 0x36);
    wslcd_write_data (self, 0x00);
    wslcd_write_reg (self, 0x3a);
    wslcd_write_data (self, 0x55);
    wslcd_write_reg (self, 0xb2);
    wslcd_write_data (self, 0x0c);
    wslcd_write_data (self, 0x0c);
    wslcd_write_data (self, 0x00);
    wslcd_write_data (self, 0x33);
    wslcd_write_data (self, 0x33);
    wslcd_write_reg (self, 0xb7);
    wslcd_write_data (self, 0x35);
    wslcd_write_reg (self, 0xbb);
    wslcd_write_data (self, 0x28);
    wslcd_write_reg (self, 0xc0);
    wslcd_write_data (self, 0x3c);
    wslcd_write_reg (self, 0xc2);
    wslcd_write_data (self, 0x01);
    wslcd_write_reg (self, 0xc3);
    wslcd_write_data (self, 0x0b);
    wslcd_write_reg (self, 0xc4);
    wslcd_write_data (self, 0x20);
    wslcd_write_reg (self, 0xc6);
    wslcd_write_data (self, 0x0f);
    wslcd_write_reg (self, 0xD0);
    wslcd_write_data (self, 0xa4);
    wslcd_write_data (self, 0xa1);
    wslcd_write_reg (self, 0xe0);
    wslcd_write_data (self, 0xd0);
    wslcd_write_data (self, 0x01);
    wslcd_write_data (self, 0x08);
    wslcd_write_data (self, 0x0f);
    wslcd_write_data (self, 0x11);
    wslcd_write_data (self, 0x2a);
    wslcd_write_data (self, 0x36);
    wslcd_write_data (self, 0x55);
    wslcd_write_data (self, 0x44);
    wslcd_write_data (self, 0x3a);
    wslcd_write_data (self, 0x0b);
    wslcd_write_data (self, 0x06);
    wslcd_write_data (self, 0x11);
    wslcd_write_data (self, 0x20);
    wslcd_write_reg (self, 0xe1);
    wslcd_write_data (self, 0xd0);
    wslcd_write_data (self, 0x02);
    wslcd_write_data (self, 0x07);
    wslcd_write_data (self, 0x0a);
    wslcd_write_data (self, 0x0b);
    wslcd_write_data (self, 0x18);
    wslcd_write_data (self, 0x34);
    wslcd_write_data (self, 0x43);
    wslcd_write_data (self, 0x4a);
    wslcd_write_data (self, 0x2b);
    wslcd_write_data (self, 0x1b);
    wslcd_write_data (self, 0x1c);
    wslcd_write_data (self, 0x22);
    wslcd_write_data (self, 0x1f);
    wslcd_write_reg (self, 0x55);
    wslcd_write_data (self, 0xB0);
    wslcd_write_reg (self, 0x29);
    }
  else
    {
    wslcd_write_reg (self, 0x21); // Display invert?
    wslcd_write_reg (self, 0xC2); // Power control 3 for normal mode
    wslcd_write_data (self, 0x33); // Can be increased
    wslcd_write_reg (self, 0XC5); // VCOM contrl (4 params)
    wslcd_write_data (self, 0x00);
    wslcd_write_data (self, 0x1e);//VCM_REG[7:0]. <=0X80.
    wslcd_write_data (self, 0x80);
    wslcd_write_reg (self, 0xB1); // Frame rate control (2 params)
    wslcd_write_data (self, 0xB0);//0XB0 =70HZ, <=0XB0.0xA0=62HZ
    wslcd_write_reg (self, 0x36); // Memory access control (one param)
    wslcd_write_data (self, 0x28); //2 DOT FRAME MODE,F<=70HZ.
    wslcd_write_reg (self, 0XE0); // Positive gamma control (15 params)
    wslcd_write_data (self, 0x0);
    wslcd_write_data (self, 0x13);
    wslcd_write_data (self, 0x18);
    wslcd_write_data (self, 0x04);
    wslcd_write_data (self, 0x0F);
    wslcd_write_data (self, 0x06);
    wslcd_write_data (self, 0x3a);
    wslcd_write_data (self, 0x56);
    wslcd_write_data (self, 0x4d);
    wslcd_write_data (self, 0x03);
    wslcd_write_data (self, 0x0a);
    wslcd_write_data (self, 0x06);
    wslcd_write_data (self, 0x30);
    wslcd_write_data (self, 0x3e);
    wslcd_write_data (self, 0x0f);                
    wslcd_write_reg (self, 0XE1); // Negative gamma control (15 params)
    wslcd_write_data (self, 0x0);
    wslcd_write_data (self, 0x13);
    wslcd_write_data (self, 0x18);
    wslcd_write_data (self, 0x01);
    wslcd_write_data (self, 0x11);
    wslcd_write_data (self, 0x06);
    wslcd_write_data (self, 0x38);
    wslcd_write_data (self, 0x34);
    wslcd_write_data (self, 0x4d);
    wslcd_write_data (self, 0x06);
    wslcd_write_data (self, 0x0d);
    wslcd_write_data (self, 0x0b);
    wslcd_write_data (self, 0x31);
    wslcd_write_data (self, 0x37);
    wslcd_write_data (self, 0x0f);
    wslcd_write_reg (self, 0X3A); // Set interface pixel format (1 param)
    wslcd_write_data (self, 0x55); // 16 bpp
    wslcd_write_reg (self, 0x11);//sleep out (leave sleep mode)
    sleep_ms(120); // Data sheet says 5msec !
    wslcd_write_reg (self, 0x29); // Display on
  wslcd_write_reg (self, 0x37);
  wslcd_write_data (self, 0);
  wslcd_write_data (self, 0);
    }
  }

/*============================================================================
  LCD_set_scan
  Sets the graphics memory scan direction, and thus set the orientation of
    the screen. Unfortunately, the ST7789 controller only supports 
    vertical scrolling, in portrait orientation. Ths method was supposed
    to work out the pixel width and height but, as we can only use portrait
    layout, these are always the same.
 ===========================================================================*/
static void wslcd_set_scan (WSLCD *self, WSLCDScanDir Scan_dir)
  {
  uint16_t MemoryAccessReg_Data = 0; // for register 0x36
  uint16_t DisFunReg_Data = 0; // for register 0xB6
  if (self->id == LCD_2_8)
    {
    // Not supported yet
    }
  else
    {
    // Assume 3.5-inch
    switch (Scan_dir) 
      {
      case WSLCD_SCAN_NORMAL:
        /* Memory access control: MY = 0, MX = 0, MV = 0, ML = 0 */
        /* Display Function control: NN = 0, GS = 0, SS = 1, SM = 0 */
        MemoryAccessReg_Data = 0x08;
        DisFunReg_Data = 0x22;
        break;
      case WSLCD_SCAN_INVERTED:
        /* Memory access control: MY = 0, MX = 0, MV = 0, ML = 0 */
        /* Display Function control: NN = 0, GS = 1, SS = 1, SM = 0 */
        MemoryAccessReg_Data = 0x08;
        DisFunReg_Data = 0x62;
        break;
      }

     self->width = LCD_3_5_WIDTH;
     self->height = LCD_3_5_HEIGHT;
     self->used_height = self->height;
     self->used_width = self->width;

    // Set the read / write scan direction of the frame memory
    wslcd_write_reg (self, 0xB6);
    // Bypass=memory rcm=DE mode rm=system dm=internal clock 
    wslcd_write_data (self, 0x00); 
    wslcd_write_data (self, DisFunReg_Data);

    wslcd_write_reg (self, 0x36);
    wslcd_write_data (self, MemoryAccessReg_Data);
    }
  }

/*============================================================================
  wslcd_set_window
  Sends the X and Y range of the data transfer to follow. This function
  _MUST_ be followed by the actual data transfer, and the size of the 
  transfer must match the X and Y range specified.
 ===========================================================================*/
void wslcd_set_window (const WSLCD *self, uint16_t Xstart, uint16_t Ystart, 
           uint16_t Xend, uint16_t Yend)
  {        
  uint16_t ys = Ystart + self->scroll_pos; 
  uint16_t ye = Yend + self->scroll_pos; 

  if (ys >= self->used_height) ys = (uint16_t)(ys - self->used_height);
  if (ye >= self->used_height) ye = (uint16_t)(ye - self->used_height);

  //if (ys > ye)
  //  printf ("ys=%d ye=%d\n", ys, ye); // XXX

  //set the X coordinates
  wslcd_write_reg (self, 0x2A); // Column address set
  // 16-bit start column
  wslcd_write_data (self, Xstart >> 8); 
  wslcd_write_data (self, Xstart & 0xff); 
  // 16-bit end column
  wslcd_write_data (self, (uint8_t)((Xend - 1) >> 8)); 
  wslcd_write_data (self, (Xend - 1) & 0xff);

  //set the Y coordinates
  wslcd_write_reg (self, 0x2B); // Page address set (row)
  // 16-bit start row
  wslcd_write_data (self, ys >> 8);
  wslcd_write_data (self, ys & 0xff);
  // 16-bit end row
  wslcd_write_data (self, (uint8_t)((ye - 1) >> 8));
  wslcd_write_data (self, (ye - 1) & 0xff);

  wslcd_write_reg (self, 0x2C); // Begin memory write
  }

/*============================================================================
  wslcd_set_cursor
 ===========================================================================*/
static void wslcd_set_cursor (const WSLCD *self, uint16_t xpoint, 
         uint16_t ypoint)
  {
  wslcd_set_window (self, xpoint, ypoint, xpoint, ypoint);
  }

/*============================================================================
  wslcd_send_repeated_word
 ===========================================================================*/
void wslcd_send_repeated_word (const WSLCD *self, uint16_t word, int len)
  {
  gpio_put (self->gpio_dc, 1);
  gpio_put (self->gpio_cs, 0);
  for (int i = 0; i < len; i++) 
    {
    wslcd_write_word_be (self, word);
    }
  gpio_put (self->gpio_cs, 1);
  }

#endif // PICO_ON_DEVICE

/*============================================================================
  wslcd_fill_area
 ===========================================================================*/
void wslcd_fill_area (const WSLCD *self, uint16_t xstart, 
         uint16_t ystart, uint16_t xend, uint16_t yend,        
         uint16_t color)
  {
#if PICO_ON_DEVICE
  if ((xend > xstart) && (yend > ystart)) 
    {
    wslcd_set_window (self, xstart , ystart , xend , yend);
    //LCD_SetColor (self, color, xend - xstart, yend - ystart);
    wslcd_send_repeated_word (self, color, (xend - xstart) * (yend - ystart));
    }
#else
  (void)self; (void)color;
  printf ("wslcd_fill_area %d %d %d %d\n", xstart, ystart, xend, yend);
#endif
  }

/*============================================================================
  wslcd_clear
 ===========================================================================*/
void wslcd_clear (WSLCD *self, uint16_t colour)
  {
#if PICO_ON_DEVICE
  //int baud = spi_get_baudrate (self->spi); 
  //printf ("baud=%d\n", baud);

  self->scroll_pos = 0;
  // Reset the HW top line
  wslcd_write_reg (self, 0x37);
  wslcd_write_data (self, 0);
  wslcd_write_data (self, 0);

  // Why does it not work if we try to clear the whole screen, not
  //   just the part used by scroll range? It doesn't even work if
  //   we first set the scroll range to the whole screen.
  _wslcd_set_scroll_area (self, 0, (uint16_t)self->height, 0);
  wslcd_fill_area (self, 0, 0, 
//       (uint16_t)self->width, (uint16_t)self->height, colour);
       (uint16_t)self->width, (uint16_t)self->actual_height, colour);
  wslcd_set_scroll_area (self);
#endif
  }

/*============================================================================
  wslcd_transfer
 ===========================================================================*/
void wslcd_transfer (const WSLCD *self, const uint16_t *buff, int len)
  {
#if PICO_ON_DEVICE
  gpio_put (self->gpio_dc, 1);
  gpio_put (self->gpio_cs, 0);
  // Ugh. We have to do this word-by-word, because we need to reverse the
  //   byte order. The WSLCD device expects big-endian 16-bit values, while
  //   the ARM uses little-endian. Since most (all?) operations on the 
  //   display are 16-bit, we have to swap bytes on every word :/
  for (int i = 0; i < len; i++) 
    {
    wslcd_write_word_be (self, *buff++);
    }
  gpio_put (self->gpio_cs, 1);
#else
  (void)self; (void)buff;
  printf ("wslcd_transfer %d\n", len);
#endif
  }

/*============================================================================
  wslcd_transfer_window
 ===========================================================================*/
void wslcd_transfer_window (const WSLCD *self, const uint16_t *buff, uint16_t w, 
        uint16_t h, uint16_t x, uint16_t y)
  {
#if PICO_ON_DEVICE
  wslcd_set_window (self, x, y, x + w, y + h); 
  int len = w * h;
  gpio_put (self->gpio_dc, 1);
  gpio_put (self->gpio_cs, 0);
  for (int i = 0; i < len; i++) 
    {
    wslcd_write_word_be (self, buff[i]);
    }
  gpio_put (self->gpio_cs, 1);
#else
  (void)self; (void)buff;
  printf ("wslcd_transfer %d %d %d %d\n", w, h, x, y);
#endif
  }

/*============================================================================
  wslcd_get_width
 ===========================================================================*/
int wslcd_get_width (const WSLCD *self)
  {
  return self->width;
  }

/*============================================================================
  wslcd_get_height
 ===========================================================================*/
int wslcd_get_height (const WSLCD *self)
  {
  return self->height;
  }

/*============================================================================
  wslcd_set_pixel
 ===========================================================================*/
void wslcd_set_pixel (const WSLCD *self, uint16_t x, uint16_t y, 
      uint16_t colour)
  {
#if PICO_ON_DEVICE
  if ((x < self->width) && (y <= self->height)) 
    {
    wslcd_set_cursor (self, x, y);
    gpio_put (self->gpio_dc, 1);
    gpio_put (self->gpio_cs, 0);

    wslcd_write_word_be (self, colour);
    gpio_put (self->gpio_cs, 1);
    }
#else
  (void)self; (void)x; (void)y; (void)colour;
#endif
  }

/*============================================================================
  wslcd_scroll_up
 ===========================================================================*/
void wslcd_scroll_up (WSLCD *self, uint16_t pixels)
  {
#if PICO_ON_DEVICE
  wslcd_write_reg (self, 0x37);

  self->scroll_pos = 
    (uint16_t) ((self->scroll_pos + pixels) % self->used_height); 

  wslcd_write_data (self, self->scroll_pos >> 8);
  wslcd_write_data (self, self->scroll_pos & 0xFF);

  //gpio_put (self->gpio_cs, 1);
#endif
  }

/*============================================================================
  _wslcd_set_scroll_area
 ===========================================================================*/
static void _wslcd_set_scroll_area (WSLCD *self, uint16_t tfa, 
         uint16_t vsa, uint16_t bfa)
  {
  wslcd_write_reg (self, 0x33);
  wslcd_write_data (self, tfa >> 8);
  wslcd_write_data (self, tfa & 0xFF);
  wslcd_write_data (self, vsa >> 8);
  wslcd_write_data (self, vsa & 0xFF);
  wslcd_write_data (self, bfa >> 8);
  wslcd_write_data (self, bfa & 0xFF);
  }

/*============================================================================
  wslcd_set_scroll_area
  *** MUST be called after the effective width and height are known. The
      defaults are unlikelty to be satisfactor ****
 ===========================================================================*/
static void wslcd_set_scroll_area (WSLCD *self)
  {
  uint16_t tfa = 0;
  uint16_t vsa = (uint16_t)self->used_height;
  uint16_t bfa = (uint16_t)(self->height - self->used_height); 
  _wslcd_set_scroll_area (self, tfa, vsa, bfa);
  }

/*============================================================================
  wslcd_set_used_area
 ===========================================================================*/
void wslcd_set_used_area (WSLCD *self, int width, int height) 
  {
  self->used_width = width;
  self->used_height = height;
  wslcd_set_scroll_area (self);
  }


/*============================================================================
  wslcd_init
 ===========================================================================*/
void wslcd_init (WSLCD *self)
  {
#if PICO_ON_DEVICE
  wslcd_gpio_init (self);

  wslcd_reset (self); //Hardware reset

  spi_set_baudrate (self->spi, (uint)self->baud_rate);
  gpio_set_function (self->gpio_sck, GPIO_FUNC_SPI);
  gpio_set_function (self->gpio_mosi, GPIO_FUNC_SPI);
  gpio_set_function (self->gpio_miso, GPIO_FUNC_SPI);

  wslcd_initreg (self);
        
  wslcd_set_scan (self, self->scan_dir);

  wslcd_set_scroll_area (self);

  sleep_ms (200);
#else
  (void)self;
  printf ("wslcd_init\n");
#endif
  }


/*============================================================================
  wslcd_new
 ===========================================================================*/
#if PICO_ON_DEVICE
WSLCD *wslcd_new (spi_inst_t *spi, uint gpio_cs, uint gpio_miso, 
    uint gpio_mosi, uint gpio_sck, uint gpio_rst, uint gpio_dc, 
    uint gpio_bl, int baud_rate, WSLCDScanDir scan_dir)
  {
  WSLCD *self = malloc (sizeof (WSLCD));
  memset (self, 0, sizeof (WSLCD));
  self->spi = spi;
  self->gpio_cs = gpio_cs;
  self->gpio_miso = gpio_miso;
  self->gpio_mosi = gpio_mosi;
  self->gpio_sck = gpio_sck;
  self->gpio_rst = gpio_rst;
  self->gpio_dc = gpio_dc;
  self->gpio_bl = gpio_bl;
  self->baud_rate = baud_rate;
  self->scan_dir = scan_dir;
  self->scroll_pos = 0;
  return self;
  }
#else
WSLCD *wslcd_new (void)
  {
  WSLCD *self = malloc (sizeof (WSLCD));
  memset (self, 0, sizeof (WSLCD));
  return self;
  }
#endif

/*============================================================================
  wslcd_destroy
 ===========================================================================*/
void wslcd_destroy (WSLCD *self)
  {
  free (self);
  }

