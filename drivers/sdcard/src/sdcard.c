/*============================================================================
 * sdcard/sdcard.c
 *
 * Implementation of the SDCard block device API that is defined in 
 * sdcard.h
 *
 * I am indebted to Carl J Kugler, whose SD driver implementation I referred
 * to extensively. There is also a good description of the protocol here:
 * http://rjhcoding.com/avrc-sd-interface-1.php
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 *
 * ==========================================================================*/

#if PICO_ON_DEVICE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <pico/mutex.h>
#include <pico/sem.h>
#include <hardware/gpio.h>
#include <hardware/spi.h>
#include <hardware/dma.h>
#include <sdcard/sdcard.h>
#include <sdcard/crc.h>
#include <syslog/syslog.h>

//#define TRACE SYSLOG_TRACE
//#define DEBUG SYSLOG_DEBUG
//#define INFO SYSLOG_INFO
//#define WARN SYSLOG_WARN

/* ==== Various SPI-related paramters and tokens ===== */

// The fill character is what we will send over SPI to clock in the
//   response from a previous command. 
#define SPI_FILL_CHAR 0xFF

// Various packet sizes, defined by the SD SPI interface
#define SDCARD_PACKET_SIZE 6
// Most commands return an R1 (one byte) response
#define R1_RESPONSE_SIZE 1
// R2 response is used by, for example, CM9 (send CSD). It will be
//   _at least_ two bytes long, but could be longer 
#define R2_RESPONSE_SIZE 2
// R3, R7 responses are returned by a few commands, like CMD8.
#define R3_R7_RESPONSE_SIZE 5

/* Expected response pattern for CMD8 */
#define CMD8_PATTERN 0xAA

// OCR register bits
 
// Bit 30 -- high capacity
#define OCR_HCS_CCS 0x40000000 
// Bit 24 -- low voltage
#define OCR_LOW_VOLTAGE 0x1000000 
// Bit 20 -- 3.3 V
#define OCR_3_3V 0x100000 

// Conventionally the SD SPI commands are numbered from zero, but the
//   value actually sent have bit 6 set; that is, the values begin
//   at 64. So SPI_CMD is a macro to adjust the zero-based commands to
//   the value to be sent. 
#define SPI_CMD(x) (0x40 | (x & 0x3f))

/* SPI response tokens */

#define SPI_DATA_RESPONSE_MASK 0x1F
// Card has accepted data 
#define SPI_DATA_ACCEPTED 0x05

/* SPI control tokens */

// Multiple block write 
#define SPI_START_BLOCK_MULTIPLE 0xFC
// Single-block read/write and multi-block read
#define SPI_START_BLOCK 0xFE

/* ===== SD protocol tuning paramters ====== */

/* Number of retries at sending CMDO to reset the card during init.
   10 seems conventional, but it shouldn't take that many. */
#define SD_CMD0_RESET_RETRIES 10

// Command timeout in msec
#define SD_COMMAND_TIMEOUT 2000

/* =====  R1 response ====== */

/* "R1" is the one-byte response received after most
   commands. The MS bit is always zero, followed by seven flag bits. 
   the idle state flag might be an expected response in some situations,
   but in most cases we expect the response byte to be zero. */

// A flag for checking the MS bit
#define R1_RESPONSE_RECV    0x80

// Response flags
#define R1_IDLE_STATE       0x01
#define R1_ERASE_RESET      0x02 
#define R1_ILLEGAL_COMMAND  0x04 
#define R1_COM_CRC_ERROR    0x08 
#define R1_ERASE_SEQUENCE_ERROR 0x10 
#define R1_ADDRESS_ERROR    0x20 
#define R1_PARAMETER_ERROR  0x40 

// 0xFF is what the hardware gives us if the card does not respond to
//   the command at all. 
#define R1_NO_RESPONSE 0xFF

/* =====  SD commands ====== */
/* Note that this list is kind-of exhaustive, to allow for future
   expansion; but only a few of these commands are actually used in the
   present implementation */

typedef enum 
  {
  CMD0_RESET = 0,                 /* Reset card to idle */
  CMD1_SEND_OP_COND = 1,          /* Not used */
  CMD6_SWITCH_FUNC = 6,           /* Not used */
  CMD8_SEND_IF_COND = 8,          /* Get supply voltage, etc */
  CMD9_SEND_CSD = 9,              /* Get Card Specific Data block */
  CMD10_SEND_CID = 10,            /* Not used */
  CMD12_STOP_TRANSMISSION = 12,   /* Force the card to stop transmission */
  CMD13_SEND_STATUS = 13,         /* Implemented, but not used */
  CMD16_SET_BLOCKLEN = 16,        /* Set block transfer length */
  CMD17_READ_SINGLE_BLOCK = 17,   /* Read single block of data */
  CMD18_READ_MULTIPLE_BLOCK = 18, 
  CMD24_WRITE_BLOCK = 24,         /* Write single block of data */
  CMD25_WRITE_MULTIPLE_BLOCK = 25,    /* Write data until stopped */
  CMD27_PROGRAM_CSD = 27,             /* Not used */
  CMD32_ERASE_WR_BLK_START_ADDR = 32, /* Not used */
  CMD33_ERASE_WR_BLK_END_ADDR = 33,   /* Not used */ 
  CMD38_ERASE = 38,      /* Implemented, but not used */
  CMD55_APP_CMD = 55,    /* The following command will be an app command */
  CMD56_GEN_CMD = 56,    /* Not used */
  CMD58_READ_OCR = 58,   /* Read OCR register */
  CMD59_CRC_ON_OFF = 59, /* Enables or disables command CRC */
  // "App" Commands
  ACMD6_SET_BUS_WIDTH = 6, /* Not used */
  ACMD13_SD_STATUS = 13,   /* Not used */
  ACMD22_SEND_NUM_WR_BLOCKS = 22,     /* Not used */
  ACMD23_SET_WR_BLK_ERASE_COUNT = 23,
  ACMD41_SD_SEND_OP_COND = 41, /* Init card */
  ACMD42_SET_CLR_CARD_DETECT = 42,
  ACMD51_SEND_SCR = 51,
  } SDCmd; 

/* =====  SDCard "class". ====== */
/* Users of this library will interact with this class using only 
   methods (functions) exposed in sdcard.h */

struct _SDCard
  {
  int drive_strength; // GPIO drive strength, e.g., 2.5 mA 
  uint gpio_cs; // The GPIO pin to which the card's chip select is connected
  uint gpio_miso; // The GPIO pin for card->Pico communication
  uint gpio_mosi; // The GPIO pin for Pico->card communication
  uint gpio_sck; // The GPIO pin for the clock
  mutex_t mutex; // A mutex for locking this object against mutliple threads
  semaphore_t sem; // Semaphore that is asserted when a DMA operation completes
  spi_inst_t *spi; // The selected Pico SPI interface (0 or 1)
  int tx_dma; // Transmit DMA channel
  int rx_dma; // Receive DMA channel
  dma_channel_config tx_dma_config; // Transit DMA config
  dma_channel_config rx_dma_config; // Receive DMA configu
  int baud_rate; // SPI baud rate
  SDCardType card_type; // Card type is determined during initialization
  uint64_t sectors; // Number of 512-byte sectors on the card
  bool driver_initialized; // Set when the driver is initialized 
  bool card_initialized; // Set when card is initialized 
  };

// Because we're using interrupts to signal the end of data transfers, 
//   we need the object that represents the SD card to be of global scope.
// This kind-of defeats the "object oriented" API I'm aiming for, but 
//   interrupt service routines have to be global. In a more complex 
//   installation, that supported multiple cards, we'd have to work out
//   which card each interrupt was destined for.
SDCard *global_sdcard;


/*============================================================================
 * sdcard_extract_bits
 * Helper function for extracting a word value that is buried in a group
 * of bits embedded in a record returned from an SD card command. The
 * msb and lsb arguments specify the starting and ending positions of the
 * relevant bits in the array 'data'. In general, except for specific
 * data transfers, data send by the SD card is not neatly aligned on
 * byte boundaries -- presumably this is to use bandwidth better.
 * ==========================================================================*/
static uint32_t sdcard_extract_bits (uint8_t *data, int msb, int lsb) 
  {
  uint32_t bits = 0;
  uint32_t size = 1 + (uint)msb - (uint)lsb;
  for (uint32_t i = 0; i < size; i++) 
    {
    uint32_t position = (uint32_t) lsb + i;
    uint32_t byte = 15 - (position >> 3);
    uint32_t bit = position & 0x7;
    uint32_t value = (data[byte] >> bit) & 1;
    bits |= value << i;
    }
  return bits;
  }

/*============================================================================
 * sdcard_lock
 * Lock a mutex to prevent concurrent access
 * ==========================================================================*/
static void sdcard_lock (SDCard *self)
  {
#ifdef TRACE
  TRACE ("locking", __FUNCTION__, __LINE__);
#endif
  mutex_enter_blocking (&(self->mutex));
  }

/*============================================================================
 * sdcard_unlock
 * Unlock the mutex that prevents concurrent access
 * ==========================================================================*/
static void sdcard_unlock (SDCard *self)
  {
#ifdef TRACE
  TRACE ("unlocking", __FUNCTION__, __LINE__);
#endif
  mutex_exit (&(self->mutex));
  }

/*============================================================================
 * sdcard_irq_handler
 * Called when a DMA transfer is complete. Release a semaphore so that
 *   the driver knows that the transfer is finished and data is available.
 * ==========================================================================*/
static void sdcard_irq_handler (SDCard *self) 
  {
  if (dma_hw->ints0 & 1u << self->rx_dma) 
    { 
    dma_hw->ints0 = 1u << self->rx_dma; // Clear interrupt flag 
    sem_release (&self->sem);
    }
  }

/*============================================================================
 * sdcard_global_irc
 * end-of-DMA interrupts come in here, and are passed to the SDCard object.
 * ==========================================================================*/
static void sdcard_global_irq (void)
  {
  sdcard_irq_handler (global_sdcard);
  }  

/*============================================================================
 * sdcard_spi_slow
 * Set a safe, low SPI baudrate when starting initialization
 * ==========================================================================*/
static void sdcard_spi_slow (SDCard *self)
  {
#ifdef DEBUG 
  uint actual = spi_set_baudrate (self->spi, 400 * 1000);
  DEBUG ("Actual frequency: %lu", (long)actual);
#else
  spi_set_baudrate (self->spi, 400 * 1000);
#endif
  }

/*============================================================================
 * sdcard_spi_fast
 * After initialization, we can switch to the (usually) higher baud rate
 *   specified when the SDCard object was created. 
 * ==========================================================================*/
static void sdcard_spi_fast (SDCard *self)
  {
#ifdef DEBUG 
  uint actual = spi_set_baudrate (self->spi, (uint)self->baud_rate);
  DEBUG ("Actual frequency: %lu", (long)actual);
#else
  spi_set_baudrate (self->spi, (uint)self->baud_rate);
#endif
  }

/*============================================================================
 * sdcard_spi_transfer
 * Initiate an SPI transfer of length bytes, sending the data from tx and
 *   reading the response into rx. The transfer is actually done using DMA,
 *   even for one byte. The DMA process will send an interrupt when it is
 *   finished, which will release a semaphone that this function monitors. 
 * Either of tx and rx can be NULL, in which case dummy values will be
 *   filled in.
 * ==========================================================================*/
static bool sdcard_spi_transfer (SDCard *self, const uint8_t *tx, uint8_t *rx, 
        size_t length)
  {
#ifdef TRACE
  // Usually too much logging, even in trace mode :)
  // TRACE ("SPI transfer of size %d", length);
  // if (tx)
  //  TRACE ("First byte is %02x", tx[0]);
#endif
  if (tx) 
    {
    channel_config_set_read_increment (&self->tx_dma_config, true);
    } 
  else 
    {
    static const uint8_t dummy = SPI_FILL_CHAR;
    tx = &dummy;
    channel_config_set_read_increment (&self->tx_dma_config, false);
    }

  if (rx) 
    {
    channel_config_set_write_increment (&self->rx_dma_config, true);
    } 
  else 
    {
    static uint8_t dummy = 0xA5;
    rx = &dummy;
    channel_config_set_write_increment (&self->rx_dma_config, false);
    }

  // Clear the interrupt flag for DMA -- we don't want any spurious
  //   interrupts. 
  dma_hw->ints0 = 1u << self->rx_dma;

  // Link the DMA chanels to the SPI system. The receive channel reads
  //   from SPI and writes to rx; the transmit channel reads from
  //   tx and writes to SPI
  dma_channel_configure ((uint)self->tx_dma, &self->tx_dma_config,
      &spi_get_hw (self->spi)->dr, tx, length, false);  
  dma_channel_configure ((uint)self->rx_dma, &self->rx_dma_config,
      rx, &spi_get_hw (self->spi)->dr, length,  false);  

  // It seems that we need to wait and start both SMA channels at the
  //   same time to avoid shift-register overruns (at least, this is
  //   what the Pico examples do). 
  dma_start_channel_mask ((1u << self->tx_dma) | (1u << self->rx_dma));

  // Wait up to one second to acquire the semaphore, indicating
  //   that the interrupt service routine has finished, and the
  //   DMA transfer is complete.
  uint32_t timeout = 1000;
  bool rc = sem_acquire_timeout_ms (&self->sem, timeout);  
  if (!rc) 
    {
    // If the timeout is reached, return false
#ifdef DEBUG 
    DEBUG ("Notification wait timed out");
#endif
    return false;
    }

  dma_channel_wait_for_finish_blocking ((uint)self->tx_dma);
  dma_channel_wait_for_finish_blocking ((uint)self->rx_dma);

#ifdef TRACE
  // Too much logging
  //TRACE ("%s DMA transfer complete\n", __FUNCTION__);
#endif
  return true;
  }

/*============================================================================
 * sdcard_send_initializing_sequence
 * See README.md for details of the card initialization sequence
 * ==========================================================================*/
static void sdcard_send_initializing_sequence (SDCard *self)
  {
#ifdef TRACE
  TRACE ("start");
#endif

  bool old_ss = gpio_get (self->gpio_cs);
  // Set DI and CS high and apply "74 or more" clock pulses to SCLK.
  // In practice, we will do this by sending ten 0xFF bytes (= 80 bits).
  gpio_put(self->gpio_cs, 1);
  uint8_t ones[10];
  memset (ones, 0xFF, sizeof ones);
  absolute_time_t timeout_time = make_timeout_time_ms(1);
  do 
    {
    sdcard_spi_transfer (self, ones, NULL, sizeof ones);
    } while (0 < absolute_time_diff_us (get_absolute_time(), timeout_time));

  gpio_put (self->gpio_cs, old_ss);
#ifdef TRACE
  TRACE ("end");
#endif
  }

/*============================================================================
 * sdcard_spi_write
 * Write a single byte using SPI. Note that this function also _reads_
 *   a single byte, when the transmitted byte is the 'fill characer 0xFF
 * ==========================================================================*/
static uint8_t sdcard_spi_write (SDCard *self, const uint8_t value) 
  {
  uint8_t received = SPI_FILL_CHAR;
  sdcard_spi_transfer (self, &value, &received, 1);
  return received;
  }

/*============================================================================
 * sdcard_cmd_spi
 * Form a command into an SPI data block, and send it using DMA
 * ==========================================================================*/
static uint8_t sd_cmd_spi (SDCard *self, SDCmd cmd, uint32_t arg) 
  {
  uint8_t response;
  char cmdPacket [SDCARD_PACKET_SIZE];

  // Prepare the command packet
  cmdPacket[0] = (char)SPI_CMD (cmd);
  cmdPacket[1] = (char)(arg >> 24);
  cmdPacket[2] = (char)(arg >> 16);
  cmdPacket[3] = (char)(arg >> 8);
  cmdPacket[4] = (char)(arg >> 0);
  cmdPacket[5] = (crc7 (cmdPacket, 5) << 1) | 0x01;
        
  // send a command
  for (int i = 0; i < SDCARD_PACKET_SIZE; i++) 
    {
    sdcard_spi_write (self, cmdPacket[i]);
    }
  // The byte received for CMD12 (stop) is a padding byte,
  // which must be discarded before we receive the rest of the CMD12 response
  if (cmd == CMD12_STOP_TRANSMISSION) 
    {
    sdcard_spi_write (self, SPI_FILL_CHAR); // Read and discard
    }
  
  // Loop for the response, which will be withing the 'NCR' time
  //   defined in the specification. The response will be of
  //   at least R1 type, that is, at least one byte. The MSB will be zero. 
  for (int i = 0; i < 0x10; i++) 
    {
    response = sdcard_spi_write (self, SPI_FILL_CHAR);
    if (!(response & R1_RESPONSE_RECV)) 
      break;
    }
  return response;
  }

/*============================================================================
 * sdcard_wait_for_ready
 * A number of operations on an SD card require that the controller wait
 *   for a specific ready signal. This is sent by reading bytes until a
 *   zero byte is received, or there is a timeout. This function
 *   returns true on "success", meaning that the ready signal was received
 *   before timeout
 * ==========================================================================*/
static bool sdcard_wait_for_ready (SDCard *self, int timeout) 
  {
  char resp;

  // Keep sending dummy clocks with DI high until the card releases the
  // DO line (i.e., we read zero)
  absolute_time_t timeout_time = make_timeout_time_ms ((uint32_t)timeout);
  do 
    {
    resp = sdcard_spi_write (self, 0xFF);
    } while (resp == 0x00 &&
             0 < absolute_time_diff_us (get_absolute_time(), timeout_time));

#ifdef WARN 
  if (resp == 0x00) WARN ("failed");
#endif 

  // Return success/failure
  return (resp > 0x00);
  }


/*============================================================================
 * sdcard_cmd
 *   Send a command, and return a 32-bit response. Each command potentially
 *   gets a different kind of response, so we have to pack the response
 *   differently according to the command. In addition, different commands have
 *   different kinds of error response, so we have to parse these here as
 *   well. These considerations make this function much more complicated than
 *   it really ought to be.
 *
 * Note that "application" ("app") commands are an extended set of commands,
 *   implemented by sending CMD55 first. 
 * ==========================================================================*/
static int sdcard_cmd (SDCard *self, const SDCmd cmd, uint32_t arg,
                  bool is_app_cmd, uint32_t *resp) 
  {
#ifdef TRACE
  TRACE ("cmd=%d arg=0x%08lx", cmd, arg);
#endif

  int32_t status = 0;
  uint32_t response;

  // No need to wait for card to be ready when sending the stop command
  if (cmd != CMD12_STOP_TRANSMISSION) 
    {
    if (!sdcard_wait_for_ready (self, SD_COMMAND_TIMEOUT)) 
      {
#ifdef WARN 
       WARN ("card not ready yet");
#endif
      }
    } // But what can we do if it does time out??
     
  // Retry command up to three times
  for (int i = 0; i < 3; i++) 
    {
    // Send CMD55 to introduce "app" command 
    if (is_app_cmd) 
      {
      response = sd_cmd_spi (self, CMD55_APP_CMD, 0x0);
      // Wait for card to be ready after CMD55
      if (!sdcard_wait_for_ready (self, SD_COMMAND_TIMEOUT)) 
        {
#ifdef WARN 
        WARN ("Card still not ready yet");
#endif
        }
      }

    // Send command over SPI 
    response = sd_cmd_spi (self, cmd, arg);
    if (response == R1_NO_RESPONSE) 
      {
#ifdef WARN 
      WARN ("no response to CMD %d", cmd);
#endif
      continue;
      }
    break;
    }

  // Pass the response to the command call if required
  if (NULL != resp) 
    {
    *resp = response;
    }
  
  if (response == R1_NO_RESPONSE) 
    {
#ifdef TRACE
    TRACE ("Response to CMD %d, response: 0x%lu\n", 
        cmd, response);
#endif
    return SD_ERR_NO_DEVICE;  
    }

  // Process the response R1  : Exit on CRC/Illegal command error/No response
  if ((response & R1_COM_CRC_ERROR) && cmd != ACMD23_SET_WR_BLK_ERASE_COUNT) 
    {
#ifdef WARN 
    WARN ("CRC error for CMD %d", cmd);
#endif
    return SD_ERR_CRC;  // CRC error
    }

  if (response & R1_ILLEGAL_COMMAND) 
    {
#ifdef WARN 
    if (cmd != ACMD23_SET_WR_BLK_ERASE_COUNT)
      WARN ("Illegal command CMD %d\n", cmd);
#endif

    if (cmd == CMD8_SEND_IF_COND) 
      {
      // Illegal command: V1 card, or not an SD Card at all
      self->card_type = SDCARD_UNKNOWN;
      }
    return SD_ERR_UNSUPPORTED;  // Command not supported
    }

  // Parse the status for other error responses
  if ((response & R1_ERASE_RESET) || (response & R1_ERASE_SEQUENCE_ERROR)) 
    {
    status = SD_ERR_ERASE;  // Erase error
    } 
  else if ((response & R1_ADDRESS_ERROR) ||
               (response & R1_PARAMETER_ERROR)) 
    {
    // Mis-aligned address or invalid address block length
    status = SD_ERR_PARAMETER;
    }

  // Form a 32-bit response part for other commands
  switch (cmd) 
    {
    case CMD8_SEND_IF_COND:  // Response R7
#ifdef DEBUG 
	 DEBUG("found V2 card");
#endif
        // If the command was CMD8, and we didn't error out earlier,
        //   then we have a V2 card.
	self->card_type = SDCARD_V2;  // fallthrough
	// Note: No break here, need to read rest of the response
    case CMD58_READ_OCR:  // Response R3
	response = (sdcard_spi_write (self, SPI_FILL_CHAR) << 24);
	response |= (sdcard_spi_write (self, SPI_FILL_CHAR) << 16);
	response |= (sdcard_spi_write (self, SPI_FILL_CHAR) << 8);
	response |= sdcard_spi_write (self, SPI_FILL_CHAR);
#ifdef dwTRACE
	TRACE ("R3/R7: 0x%ld", response);
#endif
	break;
    case CMD12_STOP_TRANSMISSION:  // Response R1b
    case CMD38_ERASE:
	sdcard_wait_for_ready (self, SD_COMMAND_TIMEOUT);
	break;
    case CMD13_SEND_STATUS:  // Response R2
        response <<= 8;
	response |= sdcard_spi_write (self, SPI_FILL_CHAR);
	if (response) 
          {
#ifdef TRACE
	  TRACE ("R2: 0x%lu\r\n", response);
#endif
	  if (response & 0x01 << 0) 
            {
#ifdef WARN 
	    WARN ("Card is locked");
#endif
	    status = SD_ERR_WRITE;
	    }
	  if (response & 0x01 << 1) 
            {
#ifdef WARN
	    WARN ("WP erase skip, lock/unlock cmd failed");
#endif
	    status = SD_ERR_WRITE_PROTECTED;
	    }
	  if (response & 0x01 << 2) 
            {
#ifdef WARN
	    WARN ("Write error");
#endif
	    status = SD_ERR_WRITE;
	    }
	  if (response & 0x01 << 3) 
            {
#ifdef WARN
	    WARN ("CRC error");
#endif
	    status = SD_ERR_WRITE;
	    }
	  if (response & 0x01 << 4) 
            {
#ifdef WARN
	    WARN ("ECC failed");
#endif
	    status = SD_ERR_WRITE;
	    }
	  if (response & 0x01 << 5) 
            {
#ifdef WARN
	    WARN ("WP violation");
#endif
	    status = SD_ERR_WRITE_PROTECTED;
	    }
	  if (response & 0x01 << 6) 
            {
#ifdef WARN
	    WARN ("Erase param");
#endif
	    status = SD_ERR_ERASE;
	    }
	  if (response & 0x01 << 7) 
            {
#ifdef WARN
	    WARN ("Out of range, CSD_Overwrite");
#endif
	    status = SD_ERR_PARAMETER;
	    }
	  if (response & 0x01 << 8) 
            {
#ifdef WARN
	    WARN ("Idle state");
#endif
	    status = 0;
	    }
	  if (response & 0x01 << 9) 
            {
#ifdef WARN
	    WARN ("Erase reset");
#endif
	    status = SD_ERR_ERASE;
	    }
	  if (response & 0x01 << 10) 
            {
#ifdef WARN
	    WARN ("Illegal command");
#endif
	    status = SD_ERR_UNSUPPORTED;
	    }
	  if (response & 0x01 << 11) 
            {
#ifdef WARN
	    WARN ("CRC error");
#endif
	    status = SD_ERR_CRC;
	    }
	  if (response & 0x01 << 12) 
            {
#ifdef WARN
	    WARN ("Erase sequence error");
#endif
	    status = SD_ERR_ERASE;
	    }
	  if (response & 0x01 << 13) 
            {
#ifdef WARN
	    WARN ("Address error");
#endif
	    status = SD_ERR_PARAMETER;
	    }
	  break;
        }
    default:  // Response R1
	break;
    }
    
  if (resp) 
    *resp = response;

  return status;
  }

/*============================================================================
 * sdcard_acquire
 * Start an SD card operation. We set chip select low to enable the card,
 *   and lock against conceurrent access. In practice, nothing in this
 *   implementation calls sdcard_lock() directly, and the lock function
 *   could probably be folded into this one. 
 * ==========================================================================*/
static void sdcard_acquire (SDCard *self)
  {
  sdcard_lock (self);
  gpio_put (self->gpio_cs, 0);
  // A fill byte seems sometimes to be necessary. Not sure why.
  uint8_t fill = SPI_FILL_CHAR;
  spi_write_blocking (self->spi, &fill, 1);
  }

/*============================================================================
 * sdcard_release
 * Finish an SD operation, by releasing the mutex lock and setting the
 *   card's chip select high
 * ==========================================================================*/
static void sdcard_release (SDCard *self)
  {
  gpio_put (self->gpio_cs, 1);
  uint8_t fill = SPI_FILL_CHAR;
  spi_write_blocking (self->spi, &fill, 1);
  sdcard_unlock (self);
  }

/*============================================================================
 * sdcard_reset
 * This is the first part of the initialization sequence. 
 * We send CMD0 until the card responds that it is in the idle state, or
 *   the operation times out
 * ==========================================================================*/
static uint32_t sdcard_reset (SDCard *self)
  {
#ifdef TRACE
      TRACE ("Start");
#endif
  uint32_t response = 0;
  for (int i = 0; i < SD_CMD0_RESET_RETRIES; i++) 
    {
#ifdef TRACE
      TRACE ("Retry %d", i);
#endif
    sdcard_cmd (self, CMD0_RESET, 0x0, false, &response);
    if (response == R1_IDLE_STATE) 
      break;

#ifdef WARN
      WARN ("Card did not go to idle state");
#endif
    // Whilst we're waiting for the next try, should we release the
    //   card and unlock? Probably we should if we're supporting anything
    //   else on the same SPI bus. Otherwise, it probably doesn't
    //   matter much.
    sdcard_release (self);
    busy_wait_us (100 * 1000); // 100 msec -- value probably not critical
    sdcard_acquire (self);
    }
#ifdef TRACE
      TRACE ("Done");
#endif
  return response;
  }

/*============================================================================
 * sdcard_wait_token
 * Read from the card until a specific start token is received, indicating that
 *   a read or write operation can begin. 
 * ==========================================================================*/
static bool sdcard_wait_start_token (SDCard *self)
  {
#ifdef TRACE
  TRACE ("Wait for start token");
#endif

  const uint32_t timeout = SD_COMMAND_TIMEOUT; 
  absolute_time_t timeout_time = make_timeout_time_ms (timeout);
  do 
    {
    if (sdcard_spi_write (self, SPI_FILL_CHAR) == SPI_START_BLOCK) 
      {
      return true;
      }
    } while (0 < absolute_time_diff_us (get_absolute_time(), timeout_time));

#ifdef WARN
  WARN ("sd_wait_token timeout");
#endif
  return false;
  }

/*============================================================================
 * sdcard_read_bytes
 * Read a specific number of bytes using SPI. This function is used for
 *   commands that return a small-ish number of bytes, rather than a 
 *   whole disk sector. carlk3's implementation works the same way, but
 *   I wonder if this function could be combined with sdard_returnread_block? 
 * ==========================================================================*/
static SDError sdcard_read_bytes (SDCard *self, 
                                    uint8_t *buffer, uint32_t length)
  {
  uint16_t crc;

  // Read until start byte (0xFE)
  if (!sdcard_wait_start_token (self)) 
    {
#ifdef WARN
    WARN ("Read timeout");
#endif
    return SD_ERR_NO_RESPONSE;
    }
    
  // read data
  for (uint32_t i = 0; i < length; i++) 
    {
    buffer[i] = sdcard_spi_write (self, SPI_FILL_CHAR);
    }
  
  // Read the CRC16 checksum for the data block
  crc = (sdcard_spi_write (self, SPI_FILL_CHAR) << 8);
  crc |= sdcard_spi_write (self, SPI_FILL_CHAR);

  uint16_t crc_result;
  // Compute and verify checksum
  crc_result = crc16 ((void *)buffer, (int)length);
  if ((uint16_t)crc_result != crc) 
    {
#ifdef WARN
    WARN ("Read_bytes: Invalid CRC");
#endif
    return SD_ERR_CRC;
    }
  return 0;
  }

/*============================================================================
 * sdcard_check_cmd8
 * Called early in card initialization, to determine whether we have a 
 * V2 card, and that it the card supports the correct voltage range. 
 * ==========================================================================*/
static SDError sdcard_check_cmd8 (SDCard *self)
  {
  uint32_t arg = (CMD8_PATTERN << 0); 
  uint32_t response = 0;
  SDError ret = 0;

  arg |= (0x1 << 8); // [11:8] supply voltage 2.7-3.6V
  // The check pattern should be 0x1AA for 3.3V cards

  ret = sdcard_cmd (self, CMD8_SEND_IF_COND, arg, false, &response);
  // Verify voltage and pattern for V2 version of card
  if ((ret == 0) && (SDCARD_V2 == self->card_type)) 
    {
    // A correct response is an echo of the original argument
    if ((response & 0xFFF) != arg) 
      {
#ifdef WARN
      WARN ("CMD8 Pattern mismatch 0x%lu 0x%lu", arg, response);
#endif
      self->card_type = SDCARD_UNKNOWN;
      ret = SD_ERR_UNUSABLE;
      }
    }
#ifdef TRACE
      TRACE ("CMD8 check correct");
#endif
  return ret; 
  }

/*============================================================================
 * sdcard_init_sector_count
 * Get the number of 512-byte sectors on the card, by parsing the 
 * card-specific data (CSD) in response to a CMD9 command. 
 * ==========================================================================*/
static SDError sdcard_init_sector_count (SDCard *self)
  {
  SDError ret = 0;

  // CMD9, Response R2 (R1 byte + 16-byte block read)
  ret = sdcard_cmd (self, CMD9_SEND_CSD, 0x0, false, 0);

  if (ret != 0)
    {
#ifdef WARN
    WARN ("CMD9 failed");
#endif
    return ret;
    }

  uint8_t csd[16];

  ret = sdcard_read_bytes (self, csd, 16);
 
  if (ret != 0)
    {
#ifdef WARN
    WARN ("Couldn't read CSD from card: %d", ret);
#endif
    return ret;
    }

  uint64_t blocks = 0, capacity = 0;
  
  // csd_structure : csd[127:126]
  uint32_t csd_structure = sdcard_extract_bits (csd, 127, 126);
  switch (csd_structure) 
    {
    case 0:
      uint32_t c_size, c_size_mult, read_bl_len;
      uint32_t block_len, mult, nblocks;
      // Older-style cards have variable block sizes, and an odd way
      //   of storing capacity. We're going to set the card transfer size
      //   to 512 bytes, whatever the card says. So to work out the number
      //   of 512-byte sectors, we must work out the capacity and then
      //   divide by 512. 
      c_size = sdcard_extract_bits (csd, 73, 62); // c_size : csd[73:62]
      c_size_mult = sdcard_extract_bits (csd, 49, 47);  // c_size_mult : csd[49:47]
      read_bl_len = sdcard_extract_bits (csd, 83, 80); // read_bl_len : csd[83:80] 
                                                       
      block_len = 1 << read_bl_len;  
      mult = 1 << (c_size_mult + 2); 
      nblocks = (c_size + 1) * mult; 
      capacity = (uint64_t)nblocks * block_len;  
      blocks = capacity / SD_BLOCK_SIZE; 
#ifdef INFO
      INFO ("Standard card: c_size: %ld", c_size);
      INFO ("Sectors: %llu", blocks);
      INFO ("Capacity: 0x%llx : %llu MB", capacity, 
        (capacity / (1024U * 1024U)));
#endif
      break;

    case 1:
      // Newer cards store the capacity in kB
      uint32_t hc_c_size = 
        sdcard_extract_bits (csd, 69, 48); // device size : C_SIZE : [69:48]
      blocks = (hc_c_size + 1) << 10;  // block count = (C_SIZE+1) * 1K
                                       
#ifdef INFO
      INFO ("SDHC/SDXC Card: hc_c_size: %lu", hc_c_size);
      INFO ("Sectors: %llu", blocks);
      INFO ("Capacity: %llu MB", (blocks / (2048U)));
#endif 
      break;

    default:
#ifdef WARN
            WARN ("CSD structure unsupported");
#endif
            return SD_ERR_UNSUPPORTED;
    };
  self->sectors = blocks;
  return 0;
  }

/*============================================================================
 * sdcard_pulse_deselect
 * When sending multiple sectors in the same card write, it has been 
 *   reported that some cards require this short deselection between
 *   blocks.
 * ==========================================================================*/
static void sdcard_pulse_deselect (SDCard *self)
  {
  uint8_t fill = SPI_FILL_CHAR;
  gpio_put (self->gpio_cs, 1);
  spi_write_blocking (self->spi, &fill, 1);
  gpio_put (self->gpio_cs, 0);
  spi_write_blocking (self->spi, &fill, 1);
  }

/*============================================================================
 * sdcard_write_block
 * Write a single sector using SPI. The return value is a result code,
 *   of which only SPI_DATA_ACCEPTED really indicates success
 * ==========================================================================*/
static uint8_t sdcard_write_block (SDCard *self, const uint8_t *buffer,
                              uint8_t token, uint32_t length) 
  {
  // Send start of block
  sdcard_spi_write (self, token);

  // Write the data
  bool ret = sdcard_spi_transfer (self, buffer, NULL, length);
  if (!ret)
    {
#ifdef WARN
    WARN ("SPI transfer failed");
#endif
    return 0;
    }

  uint16_t crc = crc16 ((void *)buffer, (int)length);

  // write the CRC 
  sdcard_spi_write (self, (uint8_t)(crc >> 8));
  sdcard_spi_write (self, (uint8_t)crc);

  // check the response token
  uint8_t response = sdcard_spi_write (self, SPI_FILL_CHAR);
   
#ifdef TRACE
    TRACE ("SPI response %02X", response);
#endif

  // Wait for last block to be written, whatever the response token
  if (!sdcard_wait_for_ready (self, SD_COMMAND_TIMEOUT)) 
    {
#ifdef WARN
    WARN ("Card not ready yet");
#endif
    }
  return (response & SPI_DATA_RESPONSE_MASK);
  }


/*============================================================================
 * sdcard_read_block
 * Having initiated a sector read, read the data using DMA
 * ==========================================================================*/
static SDError sdcard_read_block (SDCard *self, uint8_t *buffer)
  {
  if (!sdcard_wait_start_token (self))
    {
#ifdef WARN
    WARN ("Failed");
#endif
    return SD_ERR_NO_RESPONSE;
    }

 if (!sdcard_spi_transfer (self, NULL, buffer, SD_BLOCK_SIZE)) 
   {
   return SD_ERR_NO_RESPONSE;
   }

  // The data is always followed by a two-byte CRC, which we 
  //   will check.
  uint16_t rx_crc = (sdcard_spi_write (self, SPI_FILL_CHAR) << 8);
  rx_crc |= sdcard_spi_write (self, SPI_FILL_CHAR);

  uint16_t calc_crc = crc16 ((void *)buffer, SD_BLOCK_SIZE);
  if (calc_crc != rx_crc) 
    {
#ifdef WARN
    WARN ("CRC error");
#endif
    return SD_ERR_CRC;
    }

  return 0;
  }

/*============================================================================
 * sdcard_write_sectors
 * Write a number of card sectors from memory
 * ==========================================================================*/
SDError sdcard_write_sectors (SDCard *self, const uint8_t *buffer, 
         uint32_t start, uint32_t count)
  {
#ifdef TRACE
    TRACE ("Start=%lu count=%lu", start, count);
#endif

  if (!self->driver_initialized)
    {
#ifdef WARN
    WARN ("Device not initialized");
#endif
    return SD_ERR_UNINIT_DRIVER;
    }

  if (!self->card_initialized)
    {
#ifdef WARN
    WARN ("Card not initialized");
#endif
    return SD_ERR_UNINIT_CARD;
    }

  uint32_t block_count = count;
  if (start + block_count > self->sectors)
    {
#ifdef WARN
    WARN ("Sector address out of range");
#endif
    return SD_ERR_SEC_RANGE;
    }

  // Standard-capacity cards use byte-based addressing; high-capacity
  //   cards use sector addressing
  uint32_t addr;
  if (self->card_type == SDCARD_V2HC)
    {
    addr = start;
    }
  else
    {
    addr = start * SD_BLOCK_SIZE;
    }

#ifdef TRACE
    TRACE ("Card address %lu", addr);
#endif

  sdcard_acquire (self);
  SDError ret = 0;

  if (block_count == 1)
    {
    ret = sdcard_cmd (self, CMD24_WRITE_BLOCK, addr, false, 0);
    if (ret)
      {
#ifdef WARN
      WARN ("CMD24 failed");
#endif
      sdcard_release (self);
      return SD_ERR_WRITE;
      }

    uint8_t response = sdcard_write_block (self, buffer, 
             SPI_START_BLOCK, SD_BLOCK_SIZE);
    if (response != SPI_DATA_ACCEPTED)
      {
#ifdef WARN
      WARN ("Write_block failed: response=%04X", response);
#endif
      sdcard_release (self);
      // Perhaps we ought to parse the response, and check for specific
      //   errors? Still, the user probably won't be able to make any
      //   use of the details. 
      return SD_ERR_WRITE;
      }
    }
  else
    {
    // Multiple blocks

    // It seems widely believed that we can send ACMD23 to pre-erase
    //   blocks and speed up writing. The argument to this command is
    //   not very clearly documented. Given the vagueness of the situation.
    //   I'm not reporting an error if this operation fails.
    sdcard_cmd (self, ACMD23_SET_WR_BLK_ERASE_COUNT, block_count, 1, 0);
    
    sdcard_pulse_deselect (self); // I'm told that some cards need this

    // Tell the card that multiple blocks are coming, and where to
    //   store them.
    ret = sdcard_cmd (self, CMD25_WRITE_MULTIPLE_BLOCK, addr, false, 0);
    if (ret)
      {
#ifdef WARN
      WARN ("CMD25 failed");
#endif
      sdcard_release (self);
      return SD_ERR_WRITE;
      }
    
    // Now send the data blocks over SPI, one by one
    do
      {
      uint8_t response = sdcard_write_block (self, buffer, 
             SPI_START_BLOCK_MULTIPLE, SD_BLOCK_SIZE);

      if (response != SPI_DATA_ACCEPTED)
        {
#ifdef WARN
        WARN ("Write_block failed");
#endif
        sdcard_release (self);
        return SD_ERR_WRITE;
        }

      block_count--;
      buffer += SD_BLOCK_SIZE;
      } while (block_count > 0);

    }

  // Again, it's not entirely clear to me whether the following operations
  //   are actually needed, although other implementations seem to use
  //   them.
  uint32_t stat;
  sdcard_pulse_deselect (self);
  ret = sdcard_cmd (self, CMD13_SEND_STATUS, 0, false, &stat);

  sdcard_release (self);

#ifdef TRACE
    TRACE ("Done");
#endif

  return ret;
  }

/*============================================================================
 * sdcard_read_sectors
 * Read a number of card sectors into memory.
 * ==========================================================================*/
SDError sdcard_read_sectors (SDCard *self, uint8_t *buffer, uint32_t start, 
                              uint32_t count)
  {
  //printf ("Start=%lu count=%lu\n", start, count);
#ifdef TRACE
    TRACE ("Start=%lu count=%lu", start, count);
#endif

  if (!self->driver_initialized)
    {
#ifdef WARN
    WARN ("Device not initialized");
#endif
    return SD_ERR_UNINIT_DRIVER;
    }

  if (!self->card_initialized)
    {
#ifdef WARN
    WARN ("Card not initialized");
#endif
    return SD_ERR_UNINIT_CARD;
    }

  uint32_t block_count = count;
  if (start + block_count > self->sectors)
    {
#ifdef WARN
    WARN ("Sector address out of range");
#endif
    return SD_ERR_SEC_RANGE;
    }

  // Standard-capacity cards use byte-based addressing; high-capacity
  //   cards use sector addressing
  uint32_t addr;
  if (self->card_type == SDCARD_V2HC)
    {
    addr = start;
    }
  else
    {
    addr = start * SD_BLOCK_SIZE;
    }

#ifdef TRACE
    TRACE ("Card address %lu", addr);
#endif

  sdcard_acquire (self);
  SDError ret = 0;

  if (block_count > 1)
    ret = sdcard_cmd (self, CMD18_READ_MULTIPLE_BLOCK, addr, false, 0);
  else
    ret = sdcard_cmd (self, CMD17_READ_SINGLE_BLOCK, addr, false, 0);

  if (ret != 0)
    {
#ifdef WARN
    WARN ("Read command failed");
#endif
    sdcard_release (self);
    return ret;
    }

  while (block_count && (ret == 0))
    {
    ret = sdcard_read_block (self, buffer); 
    if (ret != 0)
      {
#ifdef WARN
    WARN ("sdcard_read_block failed: %d", ret);
#endif
      }
    buffer += SD_BLOCK_SIZE;
    block_count --;
    }

  // Whether the block read succeeded or failed, we need to terminate a
  //   multi-block transfer cleanly

  if (count > 1)
    {
    // What can we do if this fails? Probably nothing.
    ret = sdcard_cmd (self, CMD12_STOP_TRANSMISSION, 0, false, 0);
    }

  sdcard_release (self);

#ifdef TRACE
  TRACE ("Done");
#endif

  return ret;
  }

/*============================================================================
 * sdcard_eject_card
 * ==========================================================================*/
SDError sdcard_eject_card (SDCard *self)
  {
#ifdef TRACE
  TRACE ("Done");
#endif
  self->card_initialized = false;
  return 0;
  }

/*============================================================================
 * sdcard_insert_card
 * This call should be wrapped in sdcard_acquire/sdcard_release. This function
 *   is intended to be called by sdcard_insert_card. I'm using this wrapping
 *   because there are so many places where this function exits early, that
 *   I can't be sure I won't forget to release release the card.
 * ==========================================================================*/
static SDError _sdcard_insert_card (SDCard *self)
  {
  if (!self->driver_initialized)
    {
#ifdef WARN
    WARN ("Driver not initialized");
#endif
    return SD_ERR_UNINIT_DRIVER;
    }

  uint32_t response;

  sdcard_spi_slow (self);
  sdcard_send_initializing_sequence (self);
  
  if (sdcard_reset (self) != R1_IDLE_STATE) 
    {
#ifdef WARN
    WARN ("No disk, or could not put SD card in to SPI idle state");
#endif
    return SD_ERR_NO_DEVICE;
    }

  // Now send CMD8, and check that the response makes sense. This
  //   gives us the type of the card. The response is allowed to be
  //   'unsupported', as it will be for pre-V2 cards. 
  SDError ret = sdcard_check_cmd8 (self);
  if (ret != 0 && ret != SD_ERR_UNSUPPORTED)
    {
#ifdef WARN
    WARN ("Incorrect response to CMD8");
#endif
    return ret;
    }

  // Enable CRC
  ret = sdcard_cmd (self, CMD59_CRC_ON_OFF, 1, false, 0);
  if (ret != 0) 
    {
#ifdef WARN
    WARN ("Could not enable CRC");
#endif
    return SD_ERR_NO_CRC;
    }

  ret = sdcard_cmd (self, CMD58_READ_OCR, 0x0, false, &response);
  if (ret != 0) 
    {
#ifdef WARN
    WARN ("Read OCR failed");
#endif
    return ret;
    }
 
  if ((response & OCR_3_3V) == 0)
    {
#ifdef WARN
    WARN ("Card does not support 3.3v");
    self->card_type = SDCARD_UNKNOWN;
#endif
    return SD_ERR_UNUSABLE;
    }

  // Set the HCS flag for the ACMD41 command, to try to enable high-capacity
  //   support in the card.
  uint32_t arg = 0x0;
  if (self->card_type == SDCARD_V2) 
    arg |= OCR_HCS_CCS;

  /*  ACMD41 starts the internal initialization of the SD card itself, 
      which can take a significant time. We issue the command repeatedly, 
      until it produces a response with the idle state flag cleared 
      (that is, until the response is zero, or an error, or the process 
      times out. */

  absolute_time_t timeout_time = make_timeout_time_ms (SD_COMMAND_TIMEOUT);
  do 
    {
    ret = sdcard_cmd (self, ACMD41_SD_SEND_OP_COND, arg, true, &response);
    } while (response & R1_IDLE_STATE &&
             0 < absolute_time_diff_us (get_absolute_time(), timeout_time));

  if ((ret != 0) || (response != 0)) 
    {
    self->card_type = SDCARD_UNKNOWN;
#ifdef WARN
    WARN ("Timeout waiting for card");
#endif
    return ret;
    }

  // If we get here, we can assume that the card initialized 
  //   successfully. 

#ifdef WARN
    WARN ("Card reported initialized");
#endif

  // After all this fiddling about, let's work out what type of card
  //   we really have. We can't do this until ACMD41 has been 
  //   executed successfully, even though we did read the OCR earlier
  //   to get the voltage range. 

  if (self->card_type == SDCARD_V2)
    {
    // Is it high or low capacity?
    sdcard_cmd (self, CMD58_READ_OCR, 0, false, &response);
    if (response & OCR_HCS_CCS)
      {
      self->card_type = SDCARD_V2HC;
#ifdef DEBUG
    DEBUG ("Card type is V2 high capacity");
#endif
      }
    else
      {
#ifdef DEBUG
    DEBUG ("Card type is V2");
#endif
      }
    }
  else
    {
    // If card_type is anything other than a V2 at this stage, just
    //   assumes it's a V1.
    self->card_type = SDCARD_V1;
#ifdef TRACE
    TRACE ("Card type is V1");
#endif
    }

  ret = sdcard_init_sector_count (self);
  if (ret !=  0)
    {
#ifdef WARN
    WARN ("Init_sectors failed");
#endif
    return ret;
    }

#ifdef TRACE
    TRACE ("Sector count is %lld", self->sectors);
#endif

  ret = sdcard_cmd (self, CMD16_SET_BLOCKLEN, SD_BLOCK_SIZE, false, 0);
  if (ret != 0)
    {
#ifdef WARN
    WARN ("Set block size failed: %d", ret);
#endif
    return ret;
    }

  sdcard_spi_fast (self);

  self->card_initialized = true;

  return 0;
  }

/*============================================================================
 * sdcard_init
 * This function initialises the Pico hardware for the SD card, but does
 *   not initalize the card itself. I have made provision for this method
 *   to report an error code but, at present, no errors that might arise
 *   during this method are actually detectable. 
 * ==========================================================================*/
SDError sdcard_init (SDCard *self)
  {
  if (self->driver_initialized) return 0;

#ifdef DEBUG
  DEBUG ("CS = %d, MISO = %d, MOSI = %d, SCK = %d, rate = %d",
     self->gpio_cs, self->gpio_miso,  
     self->gpio_mosi, self->gpio_sck,
     self->baud_rate);
#endif

  global_sdcard = self;

#ifdef TRACE
  TRACE ("Start");
#endif

  if (self->drive_strength >= 0)
    {
    gpio_set_drive_strength ((enum gpio_drive_strength) 
      self->gpio_cs, self->drive_strength);
    }

  // Set the chip select line to an output
  // I'm told that we can avoid glitches on GPIO lines when changing
  //   from input to outpuot, by setting level 1 _before_ initializing.
  gpio_put (self->gpio_cs, 1);
  gpio_init (self->gpio_cs);
  gpio_set_dir (self->gpio_cs, GPIO_OUT);

  // Create a semaphore for use by the DME end-transfer interrupt
  //   handler. 
  sem_init (&(self->sem), 0, 1);
  
  // Initialize the SPI bus. The baud rate is arbitrary here, as we will
  //   set it when initialzing the card
  spi_init (self->spi, 100 * 1000);
  spi_set_format (self->spi, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

  // Set the directions for the SPI gpio pins
  gpio_set_function(self->gpio_miso, GPIO_FUNC_SPI);
  gpio_set_function(self->gpio_mosi, GPIO_FUNC_SPI);
  gpio_set_function(self->gpio_sck, GPIO_FUNC_SPI);

  if (self->drive_strength >= 0)
    {
    // If specified at construction time, set the drive strengths
    //   for the MOSI and SCK pins. In practice, 2.5mA is often
    //   sufficient but, conceivably, higher speeds might need more
    //   drive. 
    gpio_set_drive_strength ((enum gpio_drive_strength) 
      self->gpio_mosi, self->drive_strength);
    gpio_set_drive_strength ((enum gpio_drive_strength) 
      self->gpio_sck, self->drive_strength);
    }

  // The data input from the card must be pulled up, as it will probably
  //   be an open collector.
  gpio_pull_up (self->gpio_miso);

  // Set up DMA. We're using separate channels for receive and transmit
  self->tx_dma = dma_claim_unused_channel (true);
  self->rx_dma = dma_claim_unused_channel (true);
  self->tx_dma_config = dma_channel_get_default_config ((uint)self->tx_dma);
  self->rx_dma_config = dma_channel_get_default_config ((uint)self->rx_dma);

  channel_config_set_transfer_data_size (&(self->tx_dma_config), DMA_SIZE_8);
  channel_config_set_transfer_data_size (&(self->rx_dma_config), DMA_SIZE_8);

  // Link the transmit DMA channel to the relevant SPI bus
  channel_config_set_dreq  (&(self->tx_dma_config), 
    spi_get_index (self->spi) ? DREQ_SPI1_TX : DREQ_SPI0_TX);
   // setting read and write increments is a formality here -- they
   //   will be set appropriately at the start of each DMA transfer
   channel_config_set_write_increment (&self->tx_dma_config, false);

  // Link the receive DMA channel to the relevant SPI bus
  channel_config_set_dreq (&self->rx_dma_config, 
    spi_get_index(self->spi) ? DREQ_SPI1_RX : DREQ_SPI0_RX);
  channel_config_set_read_increment (&self->rx_dma_config, false);

  // Define an interrupt service routine for DMA IRQ 0
  int irq = DMA_IRQ_0;
  irq_add_shared_handler ((uint)irq, sdcard_global_irq,
    PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);

  // Turn on IRQ 0 for the DMA channel
  dma_channel_set_irq0_enabled ((uint)self->rx_dma, true);

  // ... and enable the interrupt
  irq_set_enabled ((uint)irq, true);
  self->driver_initialized = true; 

  // We've initialized this driver, but it's entirely plausible that there
  //   is no card in the slot. We could use the card-detect feature on
  //   card slots that support it, but we can't rely on this because some
  //   don't. So the only way, really, to check that there is a card in the
  //   slot is to try to initialize the card. If there is no card,  
  //   insert_card will fail early in its process -- but not instantly, because
  //   of the timeouts we have to allow.

#ifdef TRACE
  TRACE ("Done");
#endif

  return 0;
  }

/*============================================================================
 * sdcard_insert_card
 * This is the externally-accessible version of _insert_card, which
 *   wraps the insert process in acquire/release. This method should be
 *   called by client applications whenever a new card is inserted, or
 *   at start-up with a card inserted.
 * ==========================================================================*/
SDError sdcard_insert_card (SDCard *self)
  {
  sdcard_acquire (self);

  SDError ret = _sdcard_insert_card (self);

  sdcard_release (self);

  return ret;
  }

/*============================================================================
 * sdcard_get_type
 * ==========================================================================*/
SDCardType sdcard_get_type (const SDCard *self)
  {
  return self->card_type;
  }

/*============================================================================
 * sdcard_get_sectors
 * Returns the sector count
 * ==========================================================================*/
uint64_t sdcard_get_sectors (const SDCard *self)
  {
  return self->sectors;
  }

/*============================================================================
 * sdcard_perror
 * ==========================================================================*/
const char *sdcard_perror (SDError error)
  {
  switch (error)
    {
    case SD_ERR_ERASE: return "Erase failed"; // Not used
    case SD_ERR_UNSUPPORTED: return "Unsupported operation on card";
    case SD_ERR_CRC: return "CRC mismatch -- data may be corrupted";
    case SD_ERR_PARAMETER: return "Internal error: bad parameter";
    case SD_ERR_WRITE_PROTECTED: return "Card is write-protected";
    case SD_ERR_NO_DEVICE: return "No device or card";
    case SD_ERR_WRITE: return "Write failed";
    case SD_ERR_UNUSABLE: return "Card is unusable";
    case SD_ERR_NO_CRC: return "Card does not support CRC"; // Not used
    case SD_ERR_NO_RESPONSE: return "Timed out waiting for card";
    case SD_ERR_UNINIT_DRIVER: return "Driver is not initialzed";
    case SD_ERR_UNINIT_CARD: return "Card is not uninitialzed";
    case SD_ERR_SEC_RANGE: return "Sector number out of range";
    }
  
  return "Unknown error";
  }

/*============================================================================
 * sdcard_type_to_string
 * ==========================================================================*/
extern const char *sdcard_type_to_string (SDCardType type)
  {
  switch (type)
    {
    case SDCARD_NONE: return "none";
    case SDCARD_V1: return "V1 standard capacity";
    case SDCARD_V2: return "V2 standard capacity";
    case SDCARD_V2HC: return "V2 high capacity";
    default:
      return "unknown";
    }
  }

/*============================================================================
 * sdcard_get_instance
 * ==========================================================================*/
SDCard *sdcard_get_instance (int drive_num)
  {
  (void)drive_num;
  return global_sdcard;
  }

/*============================================================================
 * sdcard_is_driver_initialized
 * ==========================================================================*/
bool sdcard_is_driver_initialized (const SDCard *self)
  {
  return self->driver_initialized;
  }

/*============================================================================
 * sdcard_is_card_initialized
 * ==========================================================================*/
bool sdcard_is_card_initialized (const SDCard *self)
  {
  return self->card_initialized;
  }

/*============================================================================
 * sdcard_new
 * ==========================================================================*/
SDCard *sdcard_new (spi_inst_t *spi, int drive_strength, uint gpio_cs,
          uint gpio_miso, uint gpio_mosi, uint gpio_sck, int baud_rate)
  {
  SDCard *self = malloc (sizeof (SDCard));
  mutex_init (&(self->mutex));
  self->driver_initialized = false;
  self->card_initialized = false;
  self->spi = spi;
  self->drive_strength = drive_strength;
  self->gpio_cs = gpio_cs;
  self->gpio_miso = gpio_miso;
  self->gpio_mosi = gpio_mosi;
  self->gpio_sck = gpio_sck;
  self->baud_rate = baud_rate;
  self->card_type = SDCARD_UNKNOWN;
  return self;
  }

/*============================================================================
 * sdcard_destroy
 * ==========================================================================*/
void sdcard_destroy (SDCard* self)
  {
  free (self);
  }

#endif // PICO_ON_DEVICE
