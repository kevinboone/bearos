/*============================================================================
 *  sys/limits.h
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

#pragma once

#define PROCESS_MAX_NAME 32

// Maximum number of open files
#define NFILES 16

#define PATH_MAX 256

// max mounts cannot be < 26, since we use drive letters :)
#define FSMANAGER_MAX_MOUNTS 16

// Top of RAM -- we can use up to the conventional stack location
#define RAMTOP 0x20040000

