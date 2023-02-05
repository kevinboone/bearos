/*============================================================================
 *  compat/compat.h
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

#pragma once

/*============================================================================
 * ==========================================================================*/

// Constants for compat_fnmatch()
#define MYFNM_NOMATCH     -1
#define MYFNM_LEADING_DIR 0x0001 
#define MYFNM_NOESCAPE    0x0002 
#define MYFNM_PERIOD      0x0004 
#define MYFNM_FILE_NAME   0x0008 
#define MYFNM_CASEFOLD    0x0010 

#ifdef __cplusplus
extern "C" {
#endif

extern int compat_fnmatch (const char *pattern, const char *string, int flags);
extern char *compat_itoa (int number, char *buffer, int base);

extern void compat_printf (const char *fmt, ...);
extern void compat_printf_stderr (const char *fmt, ...);

#ifdef __cplusplus
}
#endif



