/*============================================================================
 *  fatfs_sdcard/fatloopback.h
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

#pragma once

/*============================================================================
 * ==========================================================================*/

#if PICO_ON_DEVICE
#else

#include <stdint.h>
#include <sys/error.h>
#include <sys/fsysdesc.h>

struct _FatLoopback;
typedef struct _FatLoopback FatLoopback;

#ifdef __cplusplus
extern "C" {
#endif

extern FatLoopback *fatloopback_new (void);
extern void fatloopback_destroy (FatLoopback *self);
extern FSysDescriptor *fatloopback_get_descriptor (const FatLoopback *self);

#ifdef __cplusplus
}
#endif



#endif // PICO_ON_DEVICE

