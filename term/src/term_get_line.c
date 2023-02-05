/*============================================================================
 *  term/term_get_line.c
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

/*============================================================================
 * ==========================================================================*/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pico/stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/error.h>
#include <sys/syscalls.h>
#include <errno.h>
#include <term/term.h>
#include <klib/string.h>
#include <bearos/devctl.h>
#include <bearos/terminal.h>

/*============================================================================
 * term_add_line_to_history 
 * ==========================================================================*/
static void term_write_string (const char *s)
  {
  sys_write (STDOUT_FILENO, s, (int)strlen (s));
  }

/*============================================================================
 * term_add_line_to_history 
 * ==========================================================================*/
static void term_add_line_to_history (List *history, int max_history, 
        const char *buff)
  {
  BOOL should_add = TRUE;
  int l = list_length (history);
  if (l < max_history)
    {
    for (int i = 0; i < l && should_add; i++)
      {
      if (strcmp (buff, list_get (history, i)) == 0)
        should_add = FALSE;
      }
    }

  if (should_add)
    {
    if (l >= max_history)
      {
      const char *s = list_get (history, 0);
      list_remove_object (history, s);
      }
    list_append (history, strdup (buff));
    }
  }

/*============================================================================
 * term_get_line
 * ==========================================================================*/
int term_get_line (int fd_in, char *buff, int len, 
        int max_history, List *history)
  {
  int pos = 0;
  bool done = 0;
  bool got_line = true;
  buff[0] = 0; // Make sure that something is written as the output
  // The main input buffer
  String *sbuff = string_create_empty ();
  // A copy of the main input buffer, taken when we up-arrow back
  //  into the history. We might need to restore this on a down-arrow
  char *tempstr = NULL;

  bool interrupt = false;

  int histpos = -1;
  int oldflags;
#ifdef BEAROS
  sys_devctl (0, DC_TERM_GET_FLAGS, (int32_t)&oldflags);
#else
  oldflags = DC_TERM_FLAG_ECHO; // Nasty
#endif
  
  sys_devctl (0, DC_TERM_SET_FLAGS, (int32_t)DC_TERM_FLAG_NOECHO);

  while (!done)
    {
    int c = term_get_key (fd_in);
    if (c == VK_INTR)
      {
      got_line = true; // This isn't the end of input (necessarily).
      done = true;
      interrupt = true;
      }
    else if (c == VK_EOI) 
      {
      got_line = false;
      done = true;
      }
    else if (c == VK_DEL || c == VK_BACK)
      {
      if (pos > 0) 
        {
        pos--;
        string_delete_c_at (sbuff, pos);
        putchar (O_BACKSPACE);
        const char *s = string_cstr (sbuff);
        int l = string_length (sbuff);
        for (int i = pos; i < l; i++)
          {
          putchar (s[i]);
          }
        putchar (' ');
        for (int i = pos; i <= l; i++)
          {
          putchar (O_BACKSPACE);
          }
        }
      }
    else if (c == VK_ENTER)
      {
      //buff[pos] = 0;
      done = 1;
      }
    else if (c == VK_LEFT)
      {
      if (pos > 0)
        {
        pos--;
        putchar (O_BACKSPACE);
        }
      }
    else if (c == VK_CTRLLEFT)
      {
      if (pos == 1)
        {
        pos = 0;
        putchar (O_BACKSPACE);
        }
      else
        {
        const char *s = string_cstr (sbuff);
        while (pos > 0 && isspace ((int)s[(pos - 1)]))
          {
          pos--;
          putchar (O_BACKSPACE);
          }
        while (pos > 0 && !isspace ((int)s[pos - 1]))
          {
          pos--;
          putchar (O_BACKSPACE);
          }
        }
      }
    else if (c == VK_CTRLRIGHT)
      {
      const char *s = string_cstr (sbuff);

      while (s[pos] != 0 && !isspace ((int)s[pos]))
        {
        putchar (s[pos]);
        pos++;
        }
      while (s[pos] != 0 && isspace ((int)s[pos]))
        {
        putchar (s[pos]);
        pos++;
        }
      }
    else if (c == VK_RIGHT)
      {
      const char *s = string_cstr (sbuff);
      int l = string_length (sbuff);
      if (pos < l)
        {
        putchar (s[pos]);
        pos++;
        }
      }
    else if (c == VK_UP)
      {
      if (!history) continue;
      if (histpos == 0) continue;
      int histlen = list_length (history);
      if (histlen == 0) continue;
      //printf ("histlen=%d histpos=%d\n", histlen, histpos);

      if (histpos == -1)
        {
        // We are stepping out of the main line, into the
        //  top of the history
        if (!tempstr)
            tempstr = strdup (string_cstr (sbuff));
        histpos = histlen - 1;
         }
      else
        {
        histpos --;
        }

      int oldlen = string_length (sbuff);
      const char *newline = list_get (history, histpos); 
      int newlen = (int)strlen (newline);
      // Move to the start of the line 
       for (int i = 0; i < pos; i++)
        putchar (O_BACKSPACE);
      // Write the new line
      for (int i = 0; i < newlen; i++)
        putchar (newline[i]);
      // Erase from the end of the new line to the end of the old 
      for (int i = newlen; i < oldlen; i++)
        putchar (' ');
      for (int i = newlen; i < oldlen; i++)
        putchar (O_BACKSPACE);
      pos = newlen;
      string_destroy (sbuff);
      sbuff = string_create (newline);
      }
    else if (c == VK_DOWN)
      {
      if (!history) continue;
      int histlen = list_length (history);
      if (histpos < 0) continue; 
      char *newline = "";
      bool restored_temp = false;
      if (histpos == histlen - 1)
        {
        // We're about to move off the end of the history, back to 
        //   the main line. So restore the main line, if there is 
        //   one, or just set it to "" if there is not
        histpos = -1;
        if (tempstr)
          {
          newline = tempstr;
          restored_temp = true;
          }
        }
      else
        {
        restored_temp = false;
        histpos++;
        newline = list_get (history, histpos); 
        }

      int oldlen = string_length (sbuff);
      int newlen = (int)strlen (newline);
      // Move to the start of the line 
       for (int i = 0; i < pos; i++)
          putchar (O_BACKSPACE);
      // Write the new line
      for (int i = 0; i < newlen; i++)
          putchar (newline[i]);
        // Erase from the end of the new line to the end of the old 
      for (int i = newlen; i < oldlen; i++)
          putchar (' ');
      for (int i = newlen; i < oldlen; i++)
          putchar (O_BACKSPACE);
      pos = newlen;
      string_destroy (sbuff);
      sbuff = string_create (newline);
      if (restored_temp)
        {
        free (tempstr); 
        tempstr = NULL;
        }
      }
    else if (c == VK_HOME)
      {
      if (pos > 0)
        {
        for (int i = 0; i < pos; i++)
          putchar (O_BACKSPACE);
        pos = 0;
        }
      }
    else if (c == VK_END)
      {
      const char *s = string_cstr (sbuff);
      int l = string_length (sbuff);
      for (int i = pos; i < l; i++)
          putchar (s[i]);
      pos = l;
      }
    else
      {
      int l = string_length (sbuff);

      if (l < len - 1) // Leave room for null
        {
        if ((c < 256 && c >= 32) || (c == 8))
          {
          string_insert_c_at (sbuff, pos, (char)c);
          pos++;
          int l = string_length (sbuff);
          //if (pos >= l - 1)
          if (pos >= l)
            putchar ((char)c); 
          else
            {
            const char *s = string_cstr (sbuff);
            for (int i = pos - 1; i < l; i++)
              putchar (s[i]); 
            for (int i = pos; i < l; i++)
              putchar (O_BACKSPACE); 
            }
          }
        }
      }
    }

  strncpy (buff, string_cstr(sbuff), (long unsigned int)len);
  string_destroy (sbuff);

  if (tempstr) free (tempstr);

  term_write_string (O_ENDL_STR); 
  histpos = -1; 

  sys_devctl (0, DC_TERM_SET_FLAGS, (int32_t)oldflags);

  if (got_line)
    {
    if (history && !interrupt && buff[0])
      {
      term_add_line_to_history (history, max_history, buff);
      }
    if (interrupt)
      {
      buff[0] = 0;
      return TGL_INTR;
      }
    else
      return TGL_OK;
    }
  else
    return TGL_EOI;
  }



