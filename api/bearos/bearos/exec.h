/*============================================================================
 *  api/exec.h
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/
#pragma once

// Address in RAM at which programs will be loaded
//#define BEAROS_LOAD_ADDRESS 0x20004000
//#define BEAROS_LOAD_ADDRESS 0x20005000
#define BEAROS_LOAD_ADDRESS 0x20005800

// Address at which the syscall function is stored. This will vary
//   between builds of BearOS, but the correct address should always
//   be stored at this location (just below the program load address).
//#define BEAROS_SYSCALL_VECTOR 0x20003F00
//#define BEAROS_SYSCALL_VECTOR 0x20004F00
#define BEAROS_SYSCALL_VECTOR 0x20005700

#ifdef __cplusplus
extern "C" {
#endif


#ifdef __cplusplus
}
#endif

