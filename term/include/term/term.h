/*============================================================================
 *  term/term.h
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

#pragma once

/*============================================================================
 * ==========================================================================*/

#include <stdint.h>
#include <pico/stdlib.h>
#include <sys/error.h>
#include <klib/list.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Read a key from stdin without echoing, and translate escape characters to
      virtual key codes. */
int term_get_key (int fd_in);

/** Read a line into the buffer, using simple line editing. 
    Returns true unless the user enters
    the end-of-input character (usually ctrl+d). If the user hits
    interrupt, the function returns true, but sets the interrupt flag. 
    Callers are expected to check this flag. Note that 'len' is the
    length of the buffer, not the length of the line. The line will
    be one character smaller, to allow for the string to be 
    zero-terminated. The return value is one of the TGL_XXX constants. */
int term_get_line (int fd_in, char *buff, int len, 
       int max_history, List *history);

#ifdef __cplusplus
}
#endif




