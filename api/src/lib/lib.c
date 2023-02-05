/*===========================================================================
  api/src/lib/lib.c

  This file is part of the BearOS project. 
  It mostly contains implementations of the Newlib stub functions. For
    example, if an application calls fopen(), this eventually results
    in a call to _open(), which is provided in this file. These stubs
    are, for the most part, implemented as syscalls into the BearOS 
    kernel.

  Copyright (c)2022 Kevin Boone, GPL v3.0 

===========================================================================*/
#include <stddef.h>	
#include <stdio.h>	
#include <unistd.h>	
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <errno.h>
#include <bearos/exec.h>	
#include <bearos/devctl.h>	
#include <bearos/syscalls.h>	

typedef int (*FnSyscall)(int32_t x, int32_t p1, int32_t p2, int32_t p3);
int32_t *syscall_pointer = (int32_t *)BEAROS_SYSCALL_VECTOR;

int errno__;

int _open (const char *pathname, int flags);

/*===========================================================================
  The errno.h with arm-none-eabi-gcc defines 'errno' as a function call.
  We must provide this function "__errno()". If we don't, everything builds,
  but access to "errno" causes a crash. Incidentally, this method of handling
  errno is to allow a thread-specific value; bot relevant in BearOS 
===========================================================================*/
int *__errno (void) 
  {
  return &errno__;
  }

/*===========================================================================
  syscall 
===========================================================================*/
int32_t syscall (int32_t num, int32_t arg1, int32_t arg2, int32_t arg3) 
  {
  int32_t syscall_addr = *(syscall_pointer) | 1;
  FnSyscall syscall_entry = (FnSyscall)syscall_addr;
  int32_t ret = syscall_entry (num, arg1, arg2, arg3);
  return ret;
  }

/*===========================================================================
  atoi 
===========================================================================*/
//int atoi (const char *s)
//  {
//  return (int)syscall (BEAROS_SYSCALL_ATOI, (int32_t)s, 0, 0); 
//  }

/*===========================================================================
  basename
===========================================================================*/
char *basename (char *path)
  {
  if (path == 0 || path[0] == 0) return ".";
  char *slpos = strrchr (path, '/');
  if (!slpos) return path;
  return slpos + 1;
  }

/*===========================================================================
  chdir 
===========================================================================*/
int chdir (const char *path)
  {
  int err = syscall (BEAROS_SYSCALL_CHDIR, (int32_t)path, 0, 0); 
  errno__ = err;
  return (errno__ ? -1 : 0);
  }

/*===========================================================================
  _close 
===========================================================================*/
int _close (int fd)
  {
  int err = syscall (BEAROS_SYSCALL_CLOSE, (int32_t)fd, 0, 0); 
  errno__ = err;
  return (errno__ ? -1 : 0);
  }

/*===========================================================================
  devctl 
===========================================================================*/
int devctl (int fd, intptr_t arg1, intptr_t arg2)
  {
  int err = syscall (BEAROS_SYSCALL_DEVCTL, (int32_t)fd, (int32_t)arg1,
              (int32_t) arg2); 
  errno__ = err;
  return (errno__ ? -1 : 0);
  }

/*===========================================================================
  isatty 
===========================================================================*/
int isatty (int fd)
  {
  int32_t flags = 0;
  if (devctl (fd, DC_GET_GEN_FLAGS, (int32_t)&flags) == 0)
    {
    if (flags & DC_FLAG_ISTTY) 
      return 1;
    else
      return 0;
    }
  else
    return 0;
  }

/*===========================================================================
  _exit
===========================================================================*/
__attribute__ ((noreturn)) void _exit (int status)  
  {
  syscall (BEAROS_SYSCALL_EXIT, 
    (int32_t)status, 0, 0); 
  while(1); // Never get here
  }

/*===========================================================================
  _fstat
===========================================================================*/
int _fstat (int fd, struct stat *sb)
  {
  int err = syscall (BEAROS_SYSCALL_FSTAT, 
    (int32_t)fd, (int32_t)sb, 0); 
  errno__ = err;
  return (errno__ ? -1 : 0);
  }

/*===========================================================================
  ftruncate
===========================================================================*/
int ftruncate (int fd, off_t len)
  {
  int err = syscall (BEAROS_SYSCALL_FTRUNCATE, 
    (int32_t)fd, (int32_t)len, 0); 
  errno__ = err;
  return (errno__ ? -1 : 0);
  }

/*===========================================================================
  _stat
===========================================================================*/
int _stat (const char *filename, struct stat *sb)
  {
  int fd = _open (filename, O_RDONLY);
  if (fd >= 0)
    {
    int ret = _fstat (fd, sb);
    _close (fd);
    return ret;
    }
  else
    return -1; // errno should already be set
  }


/*===========================================================================
  _getpid
===========================================================================*/
pid_t _getpid (void)
  {
  // BearOS only supports one process !
  return 1;
  }

/*===========================================================================
  _kill
===========================================================================*/
int _kill (pid_t pid, int sig)
  {
  (void)pid; (void)sig;
  // Just a stub, since we don't have any process control 
  return ESRCH;
  }

/*===========================================================================
  _gettimeofday
===========================================================================*/
int _gettimeofday (struct timeval *tv, struct timezone *tz)
  {
  syscall (BEAROS_SYSCALL_GETTIMEOFDAY, 
     (int32_t)tv, (int32_t)tz, 0); 
  return 0;
  }

/*===========================================================================
  _isatty 
===========================================================================*/
int _isatty (int fd)
  {
  (void)fd;
  return 1; // TODO TODO TODO 
  }


/*===========================================================================
  itoa 
===========================================================================*/
//char *itoa (int num, char *s, int base)
//  {
//  return (char *)syscall (BEAROS_SYSCALL_ITOA, num, (int32_t)s, base); 
//  }

/*===========================================================================
  _link
===========================================================================*/
int _link (const char *oldpath, const char *newpath)
  {
  (void)oldpath; (void)newpath;
  // We don't support links. Just return 0, and pretend we do. What
  //   alternative is there?
  return 0; 
  }



/*===========================================================================
  open 
===========================================================================*/
int _open (const char *pathname, int flags)
  {
  int fd = syscall (BEAROS_SYSCALL_OPEN, (int32_t)pathname, (int32_t)flags, 0); 
  if (fd < 0) errno__ = -fd; else errno__ = 0;
  int ret = (errno__ > 0 ? -1 : fd);
  return ret;
  }

/*===========================================================================
  _lseek 
===========================================================================*/
off_t _lseek (int fd, off_t offset, int origin)
  {
  int pos = (int)syscall (BEAROS_SYSCALL_LSEEK, 
    (int32_t)fd, (int32_t)offset, (int32_t) origin); 
  if (pos < 0) errno__ = -pos; else errno__ = 0;
  return (off_t)(pos < 0 ? -1 : pos);
  }

/*===========================================================================
  puts 
===========================================================================*/
int puts (const char *s)
  {
  int n = strlen (s);
  write (1, s, n);
  write (1, "\n", 1);
  return 1 + n;
  }

/*===========================================================================
  _putchar
  Called by printf_. Perhaps we could do some buffering here?
===========================================================================*/
void _putchar (char c)
  {
  write (1, &c, 1);
  }

/*===========================================================================
  _putchar2
  Called by printf_stderr_. Perhaps we could do some buffering here?
===========================================================================*/
void _putchar2 (char c)
  {
  write (2, &c, 1);
  }

/*===========================================================================
  _puts 
  Like puts(), but without the newline
===========================================================================*/
int _puts (const char *s)
  {
  int n = strlen (s);
  write (1, s, n);
  return n;
  }

/*===========================================================================
  read 
===========================================================================*/
int _read(int fd, void *buffer, size_t len)
  {
  int count = syscall (BEAROS_SYSCALL_READ, fd, (int32_t)buffer, len); 
  if (count < 0) errno__ = -count; else errno__ = 0;
  return (errno__ ? -1 : count);
  }

/*===========================================================================
  read_timeout 
===========================================================================*/
int read_timeout(int fd, int msec) 
  {
  return syscall (BEAROS_SYSCALL_READ_TIMEOUT, fd, msec, 0); 
  }

/*===========================================================================
  _write
===========================================================================*/
int _write (int fd, const void *buffer, size_t len)
  {
  int count = syscall (BEAROS_SYSCALL_WRITE, fd, (int32_t)buffer, len); 
  if (count < 0) errno__ = -count; else errno__ = 0;
  return (errno__ ? -1 : count);
  }

/*===========================================================================
  _sbrk 
===========================================================================*/
void *_sbrk (intptr_t increment)
  {
  //printf ("incr=%d\n", increment);
  char *ret = (void *)syscall (BEAROS_SYSCALL_SBRK, increment, 0, 0); 
  //printf ("ret=%p\n", ret);
  return ret;
  }

/*===========================================================================
  _times
===========================================================================*/
clock_t _times (struct tms *buff)
  {
  memset (buff, 0, sizeof (struct tms));
  return 0;
  }

/*===========================================================================
  _unlink
===========================================================================*/
int _unlink (char *filename)
  {
  int ret = syscall (BEAROS_SYSCALL_UNLINK, (int32_t)filename, 0, 0); 
  errno__ = ret; 
  return errno__ ? -1 : 0;
  }

/*===========================================================================
  usleep
===========================================================================*/
int usleep (useconds_t usec)
  {
  syscall (BEAROS_SYSCALL_USLEEP, usec, 0, 0); 
  return 0;
  }



