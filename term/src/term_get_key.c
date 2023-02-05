/*============================================================================
 *  term/term_get_key.c
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

/*============================================================================
 * ==========================================================================*/

#include <stdint.h>
#include <stdlib.h>
#include <pico/stdlib.h>
#include <stdio.h>
#include <sys/error.h>
#include <errno.h>
#include <term/term.h>
#include <bearos/terminal.h>
#include <sys/syscalls.h>

/*============================================================================
 * term_get_char
 * ==========================================================================*/
static int term_get_char (int fd_in)
  {
  char c;
  if (sys_read (fd_in, &c, 1) >= 1)
    return c;
  else
    return 0;
  }

/*============================================================================
 * term_get_char_timeout
 * ==========================================================================*/
static int term_get_char_timeout (int fd_in, int msec)
  {
  int c = sys_read_timeout (fd_in, msec);
  return c;
  }

/*============================================================================
 * term_get_key
 * ==========================================================================*/
int term_get_key (int fd_in)
  {
  char c = (char)term_get_char (fd_in);
  if (c == '\x1b') 
    {
    char c1 = (char)term_get_char_timeout (fd_in, I_ESC_TIMEOUT);
    if (c1 == (char)0xFF) return VK_ESC;
    //printf ("c1 a =%d %c\n", c1, c1);
    if (c1 == '[') 
      {
      char c2 = (char)term_get_char (fd_in);
      //printf ("c2 a =%d %c\n", c2, c2);
      if (c2 >= '0' && c2 <= '9') 
        {
        char c3 = (char)term_get_char(fd_in);
        //printf ("c3 a =%d %c\n", c3, c3);
        if (c3 == '~') 
          {
          switch (c2) 
            {
            case '0': return VK_END;
            case '1': return VK_HOME;
            case '2': return VK_INS;
            case '3': return VK_DEL; // Usually the key marked "del"
            case '5': return VK_PGUP;
            case '6': return VK_PGDN;
            }
          }
        else if (c3 == ';')
          {
          if (c2 == '1') 
            {
            char c4 = (char)term_get_char (fd_in); // Modifier
            char c5 = (char)term_get_char (fd_in); // Direction
            //printf ("c4 b =%d %c\n", c4, c4);
            //printf ("c5 b =%d %c\n", c5, c5);
            if (c4 == '5') // ctrl
              {
              switch (c5) 
        	{
        	case 'A': return VK_CTRLUP;
        	case 'B': return VK_CTRLDOWN;
        	case 'C': return VK_CTRLRIGHT;
        	case 'D': return VK_CTRLLEFT;
        	case 'H': return VK_CTRLHOME;
        	case 'F': return VK_CTRLEND;
        	}
              }
            else if (c4 == '2') // shift 
              {
              switch (c5) 
        	{
        	case 'A': return VK_SHIFTUP;
        	case 'B': return VK_SHIFTDOWN;
        	case 'C': return VK_SHIFTRIGHT;
        	case 'D': return VK_SHIFTLEFT;
        	case 'H': return VK_SHIFTHOME;
        	case 'F': return VK_SHIFTEND;
        	}
              }
            else if (c4 == '6') // shift-ctrl
              {
              switch (c5) 
        	{
        	case 'A': return VK_CTRLSHIFTUP;
        	case 'B': return VK_CTRLSHIFTDOWN;
        	case 'C': return VK_CTRLSHIFTRIGHT;
        	case 'D': return VK_CTRLSHIFTLEFT;
        	case 'H': return VK_CTRLSHIFTHOME;
        	case 'F': return VK_CTRLSHIFTEND;
        	}
              }
            else 
              {
              // Any other modifier, we don't support. Just return
              //   the raw direction code
              switch (c5) 
        	{
        	case 'A': return VK_UP;
        	case 'B': return VK_DOWN;
        	case 'C': return VK_RIGHT;
        	case 'D': return VK_LEFT;
        	case 'H': return VK_HOME;
        	case 'F': return VK_END;
        	}
              }
            }
          else return VK_ESC;
          }
        }
      else
        {
        //printf ("c2 b =%d %c\n", c2, c2);
        switch (c2) 
          {
          case 'A': return VK_UP;
          case 'B': return VK_DOWN;
          case 'C': return VK_RIGHT;
          case 'D': return VK_LEFT;
          case 'H': return VK_HOME;
          case 'F': return VK_END;
          case 'Z': return VK_SHIFTTAB;
          }
        }
      }
    return '\x1b';
    } 
 else 
    {
    if (c == I_BACKSPACE) return VK_BACK;
    if (c == I_DEL) return VK_DEL;
    if (c == I_INTR) return VK_INTR;
    if (c == I_EOI) return VK_EOI;
    if (c == 0) return VK_EOI;
    if (c == I_EOL) return VK_ENTER;
    return c;
    } 
  }

