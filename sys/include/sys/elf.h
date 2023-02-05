/*============================================================================
 *  sys/elf.h
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

#pragma once

#include <sys/error.h>

// Offset of the text segment in the ELF file. In the longer term we
//   need to work this out by parsing all the headers. But this
//   value will do for a proof-of-concept
//#define ELF_TEXT_OFFSET 0x5000
#define ELF_TEXT_OFFSET 0x5800

#ifdef __cplusplus
extern "C" {
#endif

/** Check that the ELF file is in a suitable format. Eventually this functino
    will return information about the load address, etc., but these things
    are all hard-coded for now. */
Error elf_check (const char *filename);

#ifdef __cplusplus
}
#endif



