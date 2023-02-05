/*============================================================================
 *  bearos/term.h
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/
#pragma once
#include <bearos/devctl.h>

/*============================================================================
 * Terminal key mappings, depending on whether we are in host or
 * Pico mode. 
 * ==========================================================================*/
// Define the ASCII code that the terminal will transmit to erase
//  the last character.
#if PICO_ON_DEVICE
#define I_BACKSPACE 8
#else
#define I_BACKSPACE 127
#endif

// Define the ASCII code that we transmit to the terminal to erase
//  the last character. This isn't necessarily the same as I_BACKSPACE
#if PICO_ON_DEVICE
#define O_BACKSPACE 8
#else
#define O_BACKSPACE 8 
#endif

#if PICO_ON_DEVICE
#define O_ENDL_STR "\r\n" 
#else
#define O_ENDL_STR "\n" 
#endif

// Define the ASCII code that the terminal will transmit to delete 
//  the last character (if it sends a code at all -- some transmit a
//  CSI sequence)
#define I_DEL 127

// Serial code for "interrupt" on the terminal. There usually isn't one,
// and we usually hit ctrl+c (ASCII 3)
#define I_INTR 3

// Serial code for "end of input" on the terminal. There usually isn't one,
// and we usually hit ctrl+d (ASCII 4)
#define I_EOI 4

// Define this if your terminal does not rub out the previous character
//   when it receives a backspace from the Pico. This setting has the
//   effect that PMBASIC will send backspace-space-backspace.
#define I_EMIT_DESTRUCTIVE_BACKSPACE

// Define how to send a end-of-line to the terminal. \n=10,
//  \r=13. Some terminals automatically expand a \n into \r\n, but most
//  do not by default.
#define I_ENDL "\r\n"

// The character that indicates an end-of-line -- usually 13 or 10 (or both)
#if PICO_ON_DEVICE
#define I_EOL 13
#else
#define I_EOL 10
#endif

// Time in milliseconds we wait after the escape key is pressed, to see
//   if it's just an escape, or the start of a CSI sequence. There is no
//   right answer here -- it depends on baud rate and other factors. Longer
//   is safer, but might irritate the user
#define I_ESC_TIMEOUT 100

/*============================================================================
 * Virtual key codes
 * ==========================================================================*/

#define VK_BACK     8
#define VK_TAB      9
#define VK_ENTER   10
#define VK_ESC     27
// Note that Linux console/terminal sends DEL when backspace is pressed
#define VK_DEL    127 
#define VK_DOWN  1000
#define VK_UP    1001 
#define VK_LEFT  1002 
#define VK_RIGHT 1003 
#define VK_PGUP  1004 
#define VK_PGDN  1005 
#define VK_HOME  1006 
#define VK_END   1007
#define VK_INS   1008
#define VK_CTRLUP 1009
#define VK_CTRLDOWN 1010
#define VK_CTRLLEFT 1011
#define VK_CTRLRIGHT 1012
#define VK_CTRLHOME 1013
#define VK_CTRLEND 1014
#define VK_SHIFTUP 1020
#define VK_SHIFTDOWN 1021
#define VK_SHIFTLEFT 1022
#define VK_SHIFTRIGHT 1023
#define VK_SHIFTHOME 1024
#define VK_SHIFTEND 1025
#define VK_SHIFTTAB 1026
#define VK_CTRLSHIFTUP 1030
#define VK_CTRLSHIFTDOWN 1031
#define VK_CTRLSHIFTLEFT 1032
#define VK_CTRLSHIFTRIGHT 1033
#define VK_CTRLSHIFTHOME 1034
#define VK_CTRLSHIFTEND 1035
#define VK_INTR 2000 
#define VK_EOI 2001 

// Return codes from terminal_get_line and related functions.
#define TGL_OK 0
// End of input
#define TGL_EOI 1
// Interrupted
#define TGL_INTR 2

/*============================================================================
 * ANSI terminal escapes 
 * ==========================================================================*/

#define TERM_ESC_HOME "\x1B[H" 
#define TERM_ESC_CLEAR_ALL "\x1B[2J" 

#define BEAROS_CONTROLLING_TTY "p:/con"

#ifdef __cplusplus
extern "C" {
#endif

extern int terminal_get_key (int fd_in);
// Returns one of the TGL_ constants
extern int terminal_get_line (int fd_in, char *buff, int len);
extern int terminal_init (int fd_in, int fd_out);
extern void terminal_reset (int fd_in, int fd_out);
extern void terminal_set_raw (int fd_in);
extern int terminal_get_props (int fd_in, DevCtlTermProps *props);

#ifdef __cplusplus
}
#endif




