/*============================================================================
 *  sdcard/crc.h
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

char crc7 (const char* data, int length);
unsigned short crc16 (const char* data, int length);

#ifdef __cplusplus
}
#endif





