/*============================================================================
 *  compat/misc.c
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

/*============================================================================
 * ==========================================================================*/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <compat/compat.h>

/*==========================================================================

  compat_fnmatch

==========================================================================*/
int compat_fnmatch (const char *pattern, const char *string, int flags)
  {
  register const char *p = pattern, *n = string;
  register char c;

#define FOLD(c)	((flags & MYFNM_CASEFOLD) ? (char)tolower (c) : (c))

  while ((c = *p++) != '\0')
    {
      c = FOLD (c);

      switch (c)
	{
	case '?':
	  if (*n == '\0')
	    return MYFNM_NOMATCH;
	  else if ((flags & MYFNM_FILE_NAME) && *n == '/')
	    return MYFNM_NOMATCH;
	  else if ((flags & MYFNM_PERIOD) && *n == '.' &&
		   (n == string || ((flags & MYFNM_FILE_NAME) && n[-1] == '/')))
	    return MYFNM_NOMATCH;
	  break;

	case '\\':
	  if (!(flags & MYFNM_NOESCAPE))
	    {
	      c = *p++;
	      c = FOLD (c);
	    }
	  if (FOLD ((char)*n) != c)
	    return MYFNM_NOMATCH;
	  break;

	case '*':
	  if ((flags & MYFNM_PERIOD) && *n == '.' &&
	      (n == string || ((flags & MYFNM_FILE_NAME) && n[-1] == '/')))
	    return MYFNM_NOMATCH;

	  for (c = *p++; c == '?' || c == '*'; c = *p++, ++n)
	    if (((flags & MYFNM_FILE_NAME) && *n == '/') ||
		(c == '?' && *n == '\0'))
	      return MYFNM_NOMATCH;

	  if (c == '\0')
	    return 0;

	  {
	    char c1 = (!(flags & MYFNM_NOESCAPE) && c == '\\') ? *p : c;
	    c1 = FOLD (c1);
	    for (--p; *n != '\0'; ++n)
	      if ((c == '[' || FOLD ((char)*n) == c1) &&
		  compat_fnmatch (p, n, flags & ~MYFNM_PERIOD) == 0)
		return 0;
	    return MYFNM_NOMATCH;
	  }

	case '[':
	  {
	    /* Nonzero if the sense of the character class is inverted.  */
	    register int negate;

	    if (*n == '\0')
	      return MYFNM_NOMATCH;

	    if ((flags & MYFNM_PERIOD) && *n == '.' &&
		(n == string || ((flags & MYFNM_FILE_NAME) && n[-1] == '/')))
	      return MYFNM_NOMATCH;

	    negate = (*p == '!' || *p == '^');
	    if (negate)
	      ++p;

	    c = *p++;
	    for (;;)
	      {
		register char cstart = c, cend = c;

		if (!(flags & MYFNM_NOESCAPE) && c == '\\')
		  cstart = cend = *p++;

		cstart = cend = FOLD (cstart);

		if (c == '\0')
		  /* [ (unterminated) loses.  */
		  return MYFNM_NOMATCH;

		c = *p++;
		c = FOLD (c);

		if ((flags & MYFNM_FILE_NAME) && c == '/')
		  /* [/] can never match.  */
		  return MYFNM_NOMATCH;

		if (c == '-' && *p != ']')
		  {
		    cend = *p++;
		    if (!(flags & MYFNM_NOESCAPE) && cend == '\\')
		      cend = *p++;
		    if (cend == '\0')
		      return MYFNM_NOMATCH;
		    cend = FOLD (cend);

		    c = *p++;
		  }

		if (FOLD ((char)*n) >= cstart
		    && FOLD ((char)*n) <= cend)
		  goto matched;

		if (c == ']')
		  break;
	      }
	    if (!negate)
	      return MYFNM_NOMATCH;
	    break;

	  matched:;
	    /* Skip the rest of the [...] that already matched.  */
	    while (c != ']')
	      {
		if (c == '\0')
		  /* [... (unterminated) loses.  */
		  return MYFNM_NOMATCH;

		c = *p++;
		if (!(flags & MYFNM_NOESCAPE) && c == '\\')
		  /* XXX 1003.2d11 is unclear if this is right.  */
		  ++p;
	      }
	    if (negate)
	      return MYFNM_NOMATCH;
	  }
	  break;

	default:
	  if (c != FOLD ((char)*n))
	    return MYFNM_NOMATCH;
	}

      ++n;
    }

  if (*n == '\0')
    return 0;

  if ((flags & MYFNM_LEADING_DIR) && *n == '/')
    /* The MYFNM_LEADING_DIR flag says that "foo*" matches "foobar/frobozz".  */
    return 0;

  return MYFNM_NOMATCH;
  }








