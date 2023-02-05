/*============================================================================

  klib 
  string.c
  Copyright (c)2017 Kevin Boone, GPL v3.0

  Methods for handling ASCII/UTF-8 strings. Be aware that these methods
  are just thin wrappers around standard, old-fashioned C library functions,
  and some will misbehave if the string actually contains multi-byte
  characters. In particular, the length() method returns the number of
  bytes, not the number of characters. Methods that search the string may
  potentially match the second or later byte of a multi-byte character.
  Any use of these methods for handling 'real' multibyte UTF-8 needs to
  be tested very carefully.

============================================================================*/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <pthread.h>
#include <stdint.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include "string.h" 
#include "shell/shell_parser.h" 
#include "../include/klib/defs.h"
#include "../include/klib/list.h"
#include "../include/klib/string.h"

struct _String
  {
  char *str;
  }; 

StringTokGlobber string_tok_globber = NULL;
StringTokVarLookup string_tok_var_lookup = NULL;

/*==========================================================================
string_create_empty 
*==========================================================================*/
String *string_create_empty (void)
  {
  return string_create ("");
  }

/*==========================================================================
string_create
*==========================================================================*/
String *string_create (const char *s)
  {
  String *self = malloc (sizeof (String));
  self->str = strdup (s);
  return self;
  }

/*==========================================================================
string_destroy
*==========================================================================*/
void string_destroy (String *self)
  {
  if (self)
    {
    if (self->str) free (self->str);
    free (self);
    }
  }


/*==========================================================================
string_cstr
*==========================================================================*/
const char *string_cstr (const String *self)
  {
  return self->str;
  }


/*==========================================================================
string_cstr_safe
*==========================================================================*/
const char *string_cstr_safe (const String *self)
  {
  if (self)
    {
    if (self->str) 
      return self->str;
    else
      return "";
    }
  else
    return "";
  }


/*==========================================================================
string_append
*==========================================================================*/
void string_append (String *self, const char *s) 
  {
  if (!s) return;
  if (self->str == NULL) self->str = strdup ("");
  int newlen = (int)strlen (self->str) + (int)strlen (s) + 2;
  self->str = realloc (self->str, (size_t)newlen);
  strcat (self->str, s);
  }


/*==========================================================================
string_prepend
*==========================================================================*/
void string_prepend (String *self, const char *s) 
  {
  if (!s) return;
  if (self->str == NULL) self->str = strdup ("");
  int newlen = (int)strlen (self->str) + (int)strlen (s) + 2;
  char *temp = strdup (self->str); 
  free (self->str);
  self->str = malloc ((size_t)newlen);
  strcpy (self->str, s);
  strcat (self->str, temp);
  free (temp);
  }


/*==========================================================================
string_append_printf
*==========================================================================*/
void string_append_printf (String *self, const char *fmt,...) 
  {
  if (self->str == NULL) self->str = strdup ("");
  va_list ap;
  va_start (ap, fmt);
  char *s;
  vasprintf (&s, fmt, ap);
  string_append (self, s);
  free (s);
  va_end (ap);
  }


/*==========================================================================
string_length
*==========================================================================*/
int32_t string_length (const String *self)
  {
  if (self == NULL) return 0;
  if (self->str == NULL) return 0;
  return (int32_t)strlen (self->str);
  }


/*==========================================================================
string_clone
*==========================================================================*/
String *string_clone (const String *self)
  {
  if (!self->str) return string_create_empty();
  return string_create (string_cstr (self));
  }


/*==========================================================================
string_find
*==========================================================================*/
int32_t string_find (const String *self, const char *search)
  {
  const char *p = strstr (self->str, search);
  if (p)
    return (int32_t)(p - self->str);
  else
    return -1;
  }


/*==========================================================================
string_find_last
*==========================================================================*/
int32_t string_find_last (const String *self, const char *search)
  {
  int lsearch = (int)strlen (search); 
  int lself = (int)strlen (self->str);
  if (lsearch > lself) return -1; // Can't find a long string in short one
  for (int i = lself - lsearch; i >= 0; i--)
    {
    BOOL diff = FALSE;
    for (int j = 0; j < lsearch && !diff; j++)
      {
      if (search[j] != self->str[i + j]) diff = TRUE;
      }
    if (!diff) return i;
    }
  return -1;
  }


/*==========================================================================
string_delete
*==========================================================================*/
void string_delete (String *self, const int pos, const int32_t len)
  {
  char *str = self->str;
  if (pos + len > (int)strlen (str))
    string_delete (self, pos, (int)strlen(str) - len);
  else
    {
    char *buff = malloc (strlen (str) - (size_t)len + 2);
    strncpy (buff, str, (size_t)pos); 
    strcpy (buff + pos, str + pos + len);
    free (self->str);
    self->str = buff;
    }
  }


/*==========================================================================
string_insert
*==========================================================================*/
void string_insert (String *self, const int pos, 
    const char *replace)
  {
  char *buff = malloc (strlen (self->str) + strlen (replace) + 2);
  char *str = self->str;
  strncpy (buff, str, (size_t)pos);
  buff[pos] = 0;
  strcat (buff, replace);
  strcat (buff, str + pos); 
  free (self->str);
  self->str = buff;
  }

/*==========================================================================
string_substitute_all
*==========================================================================*/
String *string_substitute_all (const String *self, 
    const char *search, const char *replace)
  {
  const char *gibberish = "#@x!>Aa;";
  String *working = string_clone (self);
  BOOL cont = TRUE;
  while (cont)
    {
    int i = string_find (working, search);
    if (i >= 0)
      {
      string_delete (working, i, (int)strlen (search));
      string_insert (working, i, gibberish);
      }
    else
      cont = FALSE;
    }
  cont = TRUE;
  while (cont)
    {
    int i = string_find (working, gibberish);
    if (i >= 0)
      {
      string_delete (working, i, (int)strlen (gibberish));
      string_insert (working, i, replace);
      }
    else
      cont = FALSE;
    }
  return working;
  }

/*==========================================================================
  string_encode_url
*==========================================================================*/
static char to_hex(char code)
  {
  static char hex[] = "0123456789abcdef";
  return hex[code & 15];
  }


String *string_encode_url (const char *str)
  {
  if (!str) return string_create_empty();;
  const char *pstr = str; 
  char *buf = malloc(strlen(str) * 3 + 1), *pbuf = buf;
  while (*pstr)
    {
    if (isalnum((int)*pstr) || *pstr == '-' || *pstr == '_'
      || *pstr == '.' || *pstr == '~')
      *pbuf++ = *pstr;
    else if (*pstr == ' ')
      *pbuf++ = '+';
    else
      *pbuf++ = '%', *pbuf++ = to_hex(*pstr >> 4),
         *pbuf++ = to_hex(*pstr & 15);
    pstr++;
    }
  *pbuf = '\0';
  String *result = string_create (buf);
  free (buf);
  return (result);
  }


/*==========================================================================
  string_append_byte
*==========================================================================*/
void string_append_byte (String *self, const BYTE byte)
  {
  char buff[2];
  buff[0] = (char)byte;
  buff[1] = 0;
  string_append (self, buff);
  }


/*==========================================================================
  string_append_c
*==========================================================================*/
void string_append_c (String *self, const uint32_t ch)
  {
  if (ch < 0x80) 
    {
    string_append_byte (self, (BYTE)ch);
    }
  else if (ch < 0x0800) 
    {
    string_append_byte (self, (BYTE)((ch >> 6) | 0xC0));
    string_append_byte (self, (BYTE)((ch & 0x3F) | 0x80));
    }
  else if (ch < 0x10000) 
    {
    string_append_byte (self, (BYTE)((ch >> 12) | 0xE0));
    string_append_byte (self, (BYTE)((ch >> 6 & 0x3F) | 0x80));
    string_append_byte (self, (BYTE)((ch & 0x3F) | 0x80));
    }
  else 
    {
    string_append_byte (self, (BYTE)((ch >> 18) | 0xF0));
    string_append_byte (self, (BYTE)(((ch >> 12) & 0x3F) | 0x80));
    string_append_byte (self, (BYTE)(((ch >> 6) & 0x3F) | 0x80));
    string_append_byte (self, (BYTE)((ch & 0x3F) | 0x80));
    }
  }


/*==========================================================================
  string_trim_left
*==========================================================================*/
void string_trim_left (String *self)
  {
  const char *s = self->str;
  int l = (int)strlen (s);
  int i = 0;
  int pos = 0;
  BOOL stop = FALSE;
  while (i < l && !stop)
    {
    char c = s[i];
    if (c == ' ' || c == '\n' || c == '\t')
      {
      pos++;
      }
    else
      stop = TRUE;
    i++;
    }
  char *s_new = strdup (s + pos);
  free (self->str);
  self->str = s_new;
  }


/*==========================================================================
  string_trim_right
*==========================================================================*/
void string_trim_right (String *self)
  {
  char *s = self->str;
  int l = (int)strlen (s);
  int i = l - 1;
  BOOL stop = FALSE;
  while (i >= 0 && !stop)
    {
    char c = s[i];
    if (c == ' ' || c == '\n' || c == '\t')
      {
      s[i] = 0;
      }
    else
      stop = TRUE;
    i--;
    }
  }

/*==========================================================================
  string_ends_with 
*==========================================================================*/
BOOL string_ends_with (const String *self, const char *test)
  {
  BOOL ret = FALSE;
  int pos = string_find (self, test);
  if (pos >= 0)
    {
    int lself = string_length (self);
    int ltest = (int)strlen (test);
    if (pos == lself - ltest)
      ret = TRUE;
    }
  return ret;
  }


/*==========================================================================
  string_alpha_sort_fn
  A function to use with list_sort to sort a list of strings into 
    alphabetic (ASCII) order
*==========================================================================*/
int string_alpha_sort_fn (const void *p1, const void*p2, void *user_data)
  {
  (void)user_data;
  String *s1 = * (String * const *) p1;
  String *s2 = * (String * const *) p2;
  const char *ss1 = string_cstr (s1);
  const char *ss2 = string_cstr (s2);
  return strcmp (ss1, ss2);
  }


/*==========================================================================
  string_split

  Returns a List of String objects. The string is split using strtok(),
  and so this method has all the limitations that strtok() has. In 
  particular, there's no way to enter an empty token -- multiple delimiters
  are collapsed into one.

  This method always returns a List, but it may be empty if the input
  string was empty.
*==========================================================================*/
List *string_split (const String *self, const char *delim)
  {
  List * l = list_create ((ListItemFreeFn)string_destroy);

  char *s = strdup (self->str);
  
  char *tok = strtok (s, delim);

  if (tok)
    {
    do
      {
      list_append (l, string_create (tok));
      } while ((tok = strtok (NULL, delim)));
    }

  free (s);

  return l;
  }

/*==========================================================================
  string_tokenize
*==========================================================================*/
/* tokenize() splits a string into tokens at white spaces.
 * double-quotes prevent spaces splitting the line, and an
 * escape \ before a space has the same effect. To get a quote
 * in the parsed output, use \". \" can also be used inside a 
 * quoted block -- "fred\"s" parses correctly with a " in the middle.
 * Parsing rules are similar to the shell, but not identical. In particular,
 * we don't support nested quotes
 * Caller must unref the list, which will always be valid
 * (but may be empty) */

// Tokenizer states
// Dunno state -- usually start of line where nothing has been read
#define STATE_DUNNO 0
// White state -- eating whitespace
#define STATE_WHITE 1
// General state -- eating normal chars
#define STATE_GENERAL 2
// dquote state -- last read was a double quote 
#define STATE_DQUOTE 3
// dquote state -- last read was a double quote 
#define STATE_ESC 4
// Comment state -- ignore until EOL
#define STATE_COMMENT 5
// Var state -- eating variable name 
#define STATE_VAR 6
// Var state -- eating variable name 
#define STATE_PUNCT 7

// Token types
// General char -- anything except escape, quotes, whitespace
#define CHAR_GENERAL 0
// White char -- space or tab
#define CHAR_WHITE 1
// Double quote char 
#define CHAR_DQUOTE 2
// Double quote char 
#define CHAR_ESC 3
// Hash comment
#define CHAR_HASH 4
// Variable marker
#define CHAR_VAR 5
// Redirection, pipe, etc 
#define CHAR_PUNCT 6

static void string_append_var (String *self, const char *var, void *context)
  {
  if (string_tok_var_lookup)
    { 
    const char *value = string_tok_var_lookup (var, context); 
    string_append (self, value);
    }
  else
    {
    string_append (self, "$");
    string_append (self, var);
    }
  }

void string_tok_append2 (List *args, String *tok_string, BOOL quoted, 
        int line, int col)
  {
  if (quoted)
    {
    // If it's quoted, it's a TOK_ARG, whatever the content
    Token *t = token_create (TOK_ARG, string_cstr (tok_string), line, col);
    list_append (args, t);
    }
  else
    {
    const char *_tok = string_cstr (tok_string);
    if (strcmp (_tok, ">") == 0 || strcmp (_tok, ">>") == 0 
          || strcmp (_tok, "<") == 0)
      {
      Token *t = token_create (TOK_REDIR, string_cstr (tok_string), line, col); 
      list_append (args, t);
      }
    else if (strcmp (_tok, ";") == 0)
      {
      Token *t = token_create (TOK_SEMI, string_cstr (tok_string), line, col); 
      list_append (args, t);
      }
    else if (strcmp (_tok, "|") == 0)
      {
      Token *t = token_create (TOK_PIPE, string_cstr (tok_string), line, col); 
      list_append (args, t);
      }
    else
      {
      if (string_tok_globber)
	string_tok_globber (tok_string, args); 
      else
	{
	Token *t = token_create (TOK_ARG, string_cstr (tok_string), line, col);
	list_append (args, t);
	}
      }
    }
  string_destroy (tok_string);
  }

List *string_tokenize2 (const String *s, void *context)
  {
  List *argv = list_create ((ListItemFreeFn)token_destroy);

  int i, l = (int)strlen (string_cstr(s));

  String *buff = string_create_empty();
  String *var = string_create_empty();

  int state = STATE_DUNNO;
  int last_state = STATE_DUNNO;  

  int line = 0; // TODO -- keep track of lines
  int col = 0; // Ditto

  for (i = 0; i < l; i++)
    {
    char c = s->str[i];
    int chartype = CHAR_GENERAL;
    switch (c)
      {
      case ' ': case '\t': 
        chartype = CHAR_WHITE;
        break; 

      case '"': 
        chartype = CHAR_DQUOTE;
        break; 

      case '\\': 
        chartype = CHAR_ESC;
        break; 

      case '#': 
        chartype = CHAR_HASH;
        break; 

      case '$': 
        chartype = CHAR_VAR;
        break; 

      case '>': case '<': case '|': case ';':
        chartype = CHAR_PUNCT;
        break; 

      default:
        chartype = CHAR_GENERAL;
      }

    //printf ("Got char %c of type %d in state %d\n", c, chartype, state);

    // Note: the number of cases should be num_state * num_char_types
    switch (1000 * state + chartype)
      {
      // --- Dunno states ---
      case 1000 * STATE_DUNNO + CHAR_GENERAL:
        string_append_byte (buff, (BYTE)c);
        state = STATE_GENERAL;
        break;

      case 1000 * STATE_DUNNO + CHAR_WHITE:
        // ws at start of line, or the first ws after
        state = STATE_WHITE;
        break;

      case 1000 * STATE_DUNNO + CHAR_DQUOTE:
        // Eat the quote and go into dqote mode
        state = STATE_DQUOTE;
        break;
 
      case 1000 * STATE_DUNNO + CHAR_ESC:
        last_state = state;
        state = STATE_ESC;
        break;

      case 1000 * STATE_DUNNO + CHAR_HASH:
        last_state = state;
        state = STATE_COMMENT;
        break;

      case 1000 * STATE_DUNNO + CHAR_VAR:
        last_state = state;
        state = STATE_VAR;
        break;

      case 1000 * STATE_DUNNO + CHAR_PUNCT:
        string_append_byte (buff, (BYTE)c);
        state = STATE_PUNCT;
        break;

      // --- White states ---
      case 1000 * STATE_WHITE + CHAR_GENERAL:
        // Got a char while in ws
        string_append_byte (buff, (BYTE)c);
        state = STATE_GENERAL;
        break;

      case 1000 * STATE_WHITE + CHAR_WHITE:
        // Eat ws
        break;

      case 1000 * STATE_WHITE + CHAR_DQUOTE:
        // Eat the ws and go into dqote mode
        state = STATE_DQUOTE;
        break;
 
      case 1000 * STATE_WHITE + CHAR_ESC:
        last_state = state;
        state = STATE_ESC;
        break;

      case 1000 * STATE_WHITE + CHAR_HASH:
        last_state = state;
        state = STATE_COMMENT;
        break;

      case 1000 * STATE_WHITE + CHAR_VAR:
        last_state = state;
        state = STATE_VAR;
        break;

      case 1000 * STATE_WHITE + CHAR_PUNCT:
        // Got a char while in ws
        string_append_byte (buff, (BYTE)c);
        state = STATE_PUNCT;
        break;

      // --- General states ---
      case 1000 * STATE_GENERAL + CHAR_GENERAL:
        // Eat normal char 
        string_append_byte (buff, (BYTE)c);
        break;

      case 1000 * STATE_GENERAL + CHAR_WHITE:
        //Hit ws while eating characters -- this is a token
        if (strlen (buff->str))
          //list_append (argv, buff);
          string_tok_append2 (argv, buff, FALSE, line, col);
        buff = string_create_empty();
        state = STATE_WHITE;
        break;

      case 1000 * STATE_GENERAL + CHAR_DQUOTE:
        // Go into dquote mode 
        state = STATE_DQUOTE;
        break;

      case 1000 * STATE_GENERAL + CHAR_ESC:
        last_state = state;
        state = STATE_ESC;
        break;

      case 1000 * STATE_GENERAL + CHAR_HASH:
        //Hit hash while eating characters -- this is a token
        if (strlen (buff->str))
          //list_append (argv, buff);
          string_tok_append2 (argv, buff, FALSE, line, col);
        buff = string_create_empty();
        state = STATE_COMMENT;
        break;

      case 1000 * STATE_GENERAL + CHAR_VAR:
        state = STATE_VAR;
        break;

      case 1000 * STATE_GENERAL + CHAR_PUNCT:
        //Hit >, < while eating characters -- this is part of a redir token
        string_tok_append2 (argv, buff, FALSE, line, col);
        buff = string_create_empty();
        string_append_byte (buff, (BYTE)c);
        state = STATE_PUNCT;
        break;

      // --- Dquote states ---
      case 1000 * STATE_DQUOTE + CHAR_GENERAL:
        // Store the char, but remain in dquote mode
        string_append_byte (buff, (BYTE)c);
        break;

      case 1000 * STATE_DQUOTE + CHAR_WHITE:
        // Store the ws, and remain in dquote mode
        string_append_byte (buff, (BYTE)c);
        break;

      case 1000 * STATE_DQUOTE + CHAR_DQUOTE:
        // Leave duote mode and store token (which might be empty) 
        string_tok_append2 (argv, buff, TRUE, line, col); 
        //list_append (argv, buff);
        buff = string_create_empty();
        state = STATE_DUNNO;
        break;

      case 1000 * STATE_DQUOTE + CHAR_ESC:
        //last_state = state;
        //state = STATE_ESC;
        string_append_byte (buff, (BYTE)c);
        break;

      case 1000 * STATE_DQUOTE + CHAR_HASH:
        // Keep this hash char -- it is quoted 
        string_append_byte (buff, (BYTE)c);
        break;

      case 1000 * STATE_DQUOTE + CHAR_VAR:
        // Keep this  char -- it is quoted 
        string_append_byte (buff, (BYTE)c);
        break;

      case 1000 * STATE_DQUOTE + CHAR_PUNCT:
        // Keep this  char -- it is quoted 
        string_append_byte (buff, (BYTE)c);
        break;

      // --- Esc states ---
      case 1000 * STATE_ESC + CHAR_GENERAL:
        string_append_byte (buff, (BYTE)c);
        state = last_state; 
        break;

      case 1000 * STATE_ESC + CHAR_WHITE:
        string_append_byte (buff, (BYTE)c);
        state = last_state; 
        break;

      case 1000 * STATE_ESC + CHAR_DQUOTE:
        string_append_byte (buff, (BYTE)'"');
        state = last_state; 
        break;

      case 1000 * STATE_ESC + CHAR_ESC:
        string_append_byte (buff, (BYTE)'\\');
        state = last_state; 
        break;

      case 1000 * STATE_ESC + CHAR_HASH:
        string_append_byte (buff, (BYTE)'#');
        state = last_state; 
        break;

      case 1000 * STATE_ESC + CHAR_VAR:
        string_append_byte (buff, (BYTE)'$');
        state = last_state; 
        break;

      case 1000 * STATE_ESC + CHAR_PUNCT:
        string_append_byte (buff, (BYTE)c);
        state = STATE_GENERAL; 
        break;


      // --- Comment states ---
      case 1000 * STATE_COMMENT + CHAR_GENERAL:
        break;

      case 1000 * STATE_COMMENT + CHAR_WHITE:
        break;

      case 1000 * STATE_COMMENT + CHAR_DQUOTE:
        break;

      case 1000 * STATE_COMMENT + CHAR_ESC:
        break;

      case 1000 * STATE_COMMENT + CHAR_HASH:
        break;
      
      case 1000 * STATE_COMMENT + CHAR_VAR:
        break;

      case 1000 * STATE_COMMENT + CHAR_PUNCT:
        break;
      
      // --- Var states ---

      case 1000 * STATE_VAR + CHAR_GENERAL:
      case 1000 * STATE_VAR + CHAR_PUNCT:
        // Eat normal as var name 
        if (isalnum (c) || c == '_')
          string_append_byte (var, (BYTE)c);
        else if (c == '{')
          {
          // Just skip it
          }
        else
          {
          string_append_var (buff, var->str, context);
          if (c != '}')
            string_append_byte (buff, (BYTE)c);
          string_destroy (var);
          var = string_create_empty();
          state = STATE_GENERAL;
          }
        break;

      case 1000 * STATE_VAR + CHAR_WHITE:
        // Hit ws while eating characters -- this is a var name 
        if (strlen (var->str))
          {
          string_append_var (buff, var->str, context);
          string_tok_append2 (argv, buff, FALSE, line, col);
          string_destroy (var);
          var = string_create_empty();
          buff = string_create_empty();
          } 
        else
          {
          String *dollar = string_create ("$");
          string_tok_append2 (argv, dollar, FALSE, line, col);
          }
        state = STATE_WHITE;
        break;

      case 1000 * STATE_VAR + CHAR_DQUOTE:
        state = STATE_DQUOTE;
        break;

      case 1000 * STATE_VAR + CHAR_ESC:
        string_append_var (buff, var->str, context);
        string_destroy (var);
        var = string_create_empty();
        state = STATE_GENERAL;
        break;

      case 1000 * STATE_VAR + CHAR_HASH:
        if (strlen (var->str))
          {
          string_append_var (buff, var->str, context);
          string_tok_append2 (argv, buff, FALSE, line, col);
          string_destroy (var);
          var = string_create_empty();
          buff = string_create_empty();
          } 
        else
          {
          String *dollar = string_create ("$");
          string_tok_append2 (argv, dollar, FALSE, line, col);
          }
        state = STATE_COMMENT;
        break;

      case 1000 * STATE_VAR + CHAR_VAR:
        if (strlen (var->str))
          {
          string_append_var (buff, var->str, context);
          string_destroy (var);
          var = string_create_empty();
          } 
        else
          {
          String *dollar = string_create ("$");
          string_tok_append2 (argv, dollar, FALSE, line, col);
          }
        break;
    
      // Redir states
      
      case 1000 * STATE_PUNCT + CHAR_GENERAL:
        string_tok_append2 (argv, buff, FALSE, line, col);
        buff = string_create_empty();
        string_append_byte (buff, (BYTE)c);
        state = STATE_GENERAL;
        break;

      case 1000 * STATE_PUNCT + CHAR_WHITE:
        string_tok_append2 (argv, buff, FALSE, line, col);
        buff = string_create_empty();
        state = STATE_WHITE;
        break;

      case 1000 * STATE_PUNCT + CHAR_DQUOTE:
        string_tok_append2 (argv, buff, FALSE, line, col);
        buff = string_create_empty();
        state = STATE_DQUOTE;
        break;

      case 1000 * STATE_PUNCT + CHAR_ESC:
        string_tok_append2 (argv, buff, FALSE, line, col);
        buff = string_create_empty();
        state = STATE_ESC;
        break;

      case 1000 * STATE_PUNCT + CHAR_HASH:
        string_tok_append2 (argv, buff, FALSE, line, col);
        buff = string_create_empty();
        state = STATE_COMMENT;
        break;
      
      case 1000 * STATE_PUNCT + CHAR_VAR:
        string_tok_append2 (argv, buff, FALSE, line, col);
        buff = string_create_empty();
        state = STATE_VAR;
        break;

      case 1000 * STATE_PUNCT + CHAR_PUNCT:
        string_append_byte (buff, (BYTE)c);
        break;

      default:
        printf // TODO 
           ("Internal error: bad state %d and char type %d in tokenize()",
          state, chartype);
      }
    col++;
    }

  if (state == STATE_VAR)
    {
    string_append_var (buff, var->str, context);
    string_tok_append2 (argv, buff, FALSE, line, col);
    }
  else
    {
    if (string_length (buff) > 0)
      string_tok_append2 (argv, buff, FALSE, line, col);
    else
      string_destroy (buff);
    }

/*
  if (state == STATE_GENERAL || state == STATE_PUNCT)
    {
    if (string_length (buff) > 0)
      {
      string_tok_append2 (argv, buff, FALSE, line, col);
      }
    else
      string_destroy (buff);
    }
  else if (state == STATE_VAR)
    {
    string_append_var (buff, var->str, context);
    string_tok_append2 (argv, buff, FALSE, line, col);
    }
  else if (state == STATE_DQUOTE)
    {
    string_tok_append2 (argv, buff, TRUE, line, col);
    }
  else if (state == STATE_ESC) 
    {
    if (string_length (buff) > 0)
      {
      string_tok_append2 (argv, buff, FALSE, line, col);
      }
    else
      string_destroy (buff);
    }
  else if (state == STATE_WHITE) 
    {
    if (string_length (buff) > 0)
      {
      string_tok_append2 (argv, buff, FALSE, line, col);
      }
    else
      string_destroy (buff);
    }
  else if (state == STATE_DUNNO 
            || state == STATE_WHITE) 
    {
    string_destroy (buff);
    }
*/

  Token *t = token_create (TOK_EOI, NULL, line, col); 
  list_append (argv, t);

  string_destroy (var);

  return argv;
  }


/*==========================================================================
  string_delete_last
*==========================================================================*/
void string_delete_last (String *self)
  {
  int l = (int)strlen (self->str);
  if (l > 0)
    self->str[l - 1] = 0;  
  }

/*==========================================================================
  string_insert_c_at
*==========================================================================*/
void string_insert_c_at (String *self, int pos, char c)
  {
  static char s[2] = " ";
  s[0] = c;
  string_insert (self, pos, s);
  }

/*==========================================================================
  string_delete_c_at
*==========================================================================*/
void string_delete_c_at (String *self, int pos)
  {
  string_delete (self, pos, 1); 
  }



