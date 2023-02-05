/*============================================================================
 *  fsmanager/fat.h
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

#pragma once

/*============================================================================
 * ==========================================================================*/

#include <sys/error.h>
#include <ff.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

extern Error fat_fresult_to_error (FRESULT err);
extern void fat_remove_trailing_slash (char *path);
extern uint8_t fat_open_to_f_open_mode (int flags);
extern time_t fat_time_to_unix (DWORD ftime);
extern DWORD fat_unix_to_time (time_t t);

#ifdef __cplusplus
}
#endif

