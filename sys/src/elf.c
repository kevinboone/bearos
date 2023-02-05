/*============================================================================
 *  sys/process.c
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <syslog/syslog.h>
#include <sys/syscalls.h>
#include <sys/error.h>
#include <errno.h>
#include <sys/elf.h>

#define TRACE_IN SYSLOG_TRACE_IN
#define TRACE_OUT SYSLOG_TRACE_OUT
#define TRACE SYSLOG_TRACE
#define DEBUG SYSLOG_DEBUG
#define INFO SYSLOG_INFO
#define WARN SYSLOG_WARN

typedef struct _ELFHeader
  {
  char magic[4];
  char width;
  char endian;
  char elf_version1;
  char undefined[9];

  int16_t e_type;
  int16_t e_machine;
  int32_t e_version;
  int32_t e_entry;
  int32_t e_phoff;
  int32_t e_shoff;
  int32_t e_flags;
  int16_t e_ehsize;
  int16_t e_phentsize;
  int16_t e_phnum;
  int16_t e_shentsize;
  int16_t e_shnum;
  int16_t e_shstrndx;
  } ELFHeader;


/*============================================================================
 * elf_check 
 * ==========================================================================*/
Error elf_check (const char *filename)
  {
#ifdef TRACE
  TRACE_IN;
#endif

  Error ret = 0;

  int fd = sys_open (filename, O_RDONLY);
  if (fd >= 0)
    {
    ELFHeader eh;
    int n = sys_read (fd, &eh, sizeof (eh));
    if (n == sizeof (eh))
      {
      if (eh.magic[1] == 'E' && eh.magic[2] == 'L' && eh.magic[3] == 'F'
        && eh.endian == 1)
          {
          if (eh.e_machine == 0x28)
            ret = 0;
          else
            {
            WARN ("ELF incorrect architecture: %s (%d)", 
              filename, eh.e_machine);
            ret = ENOEXEC;
            }
          }
      else
        {
        WARN ("ELF header incorrect: %s", filename);
        ret = ENOEXEC;
        }
      }
    else
      {
      WARN ("Executable too short: %s", filename);
      ret = ENOEXEC;
      }
    sys_close (fd);
    }
  else
    ret = -ENOENT;

#ifdef TRACE
  TRACE_OUT;
#endif
  return ret;
  }


