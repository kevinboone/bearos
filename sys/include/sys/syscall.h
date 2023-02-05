/*============================================================================
 *  sys/syscall.h
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

intptr_t syscall (intptr_t num, intptr_t arg1, intptr_t arg2, intptr_t arg3);

#ifdef __cplusplus
}
#endif


