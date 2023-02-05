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
#include <sys/syscall.h>
#include <bearos/syscalls.h>

// **** NOTE ****
// The order of syscalls in the syscall table must match the syscall
//   index numbers in bearos/syscalls.h

typedef intptr_t (*SyscallFn) (intptr_t arg1, intptr_t arg2, 
                  intptr_t arg3);

SyscallFn syscall_table[100] = 
  {
  0, // 0
  _sys_open, // 1
  _sys_write,  // 2
  _sys_read,  // 3
  _sys_close, // 4
  _sys_read_timeout, // 5
  _sys_fstat, // 6
  _sys_lseek, // 7
  _sys_unlink, // 8
  _sys_utime, // 9

  _sys_getdent, // 10 
  _sys_chdir, // 11 
  _sys_getcwd, // 12 
  _sys_mkdir, // 13 
  _sys_rmdir, // 14 
  _sys_ftruncate, // 15 
  0, // 16 
  0, // 17 
  0, // 18 
  0, // 19 

  _sys_poll_interrupt, // 20 
  _sys_clear_interrupt, // 21 
  _sys_get_key, // 22 
  _sys_get_line, // 23 
  0, // 24 
  0, // 25 
  0, // 26 
  0, // 27 
  0, // 28 
  0, // 29 

  _sys_access, // 30 
  0, // 31 
  0, // 32 
  0, // 33 
  0, // 34 
  0, // 35 
  0, // 36 
  0, // 37 
  0, // 38 
  0, // 39
 
  _sys_sbrk, // 40 
  _sys_exit, // 41 
  _sys_devctl, // 42 
  _sys_usleep, // 43 
  0, // 44 
  0, // 45 
  0, // 46 
  0, // 47 
  0, // 48 
  0, // 49
 
  0, //_sys_strlen, // 50 
  0, //_sys_itoa, // 51 
  0, //_sys_strerror, // 52 
  0, //_sys_atoi, // 53 
  0, //_sys_strtol, // 54 
  0, //_sys_strcmp, // 55 
  0, //_sys_strncmp, // 56 
  0, // 57 
  0, // 58 
  0, // 59 

  0, // 60 
  0, // 61 
  0, // 62 
  0, // 63 
  0, // 64 
  0, // 65 
  0, // 66 
  0, // 67 
  0, // 68 
  0, // 69 

  _sys_gmtime_r, // 70 
  _sys_gettimeofday, // 71 
  _sys_localtime_r, // 72 
  0, // 73 
  0, // 74 
  0, // 75 
  0, // 76 
  0, // 77 
  0, // 78 
  0, // 79 

  0, // 80 
  };

/*============================================================================
 * syscall 
 * ==========================================================================*/
intptr_t syscall (intptr_t num, intptr_t arg1, intptr_t arg2, intptr_t arg3)
  {
  //printf ("SYSCALL!!!! %d %d %s %d\n", num, arg1, (char *)arg2, arg3);
  
  // TODO check bounds
  SyscallFn fn = syscall_table[num];
  if (fn)
    return fn (arg1, arg2, arg3);
  else
    return EINVAL;
  }




