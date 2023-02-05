/*============================================================================
 *  sys/syscalls.h
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/
#pragma once

#include <utime.h>
#include <sys/direntry.h>
#include <sys/stat.h>
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

int sys_open (const char *path, int flags);
intptr_t _sys_open (intptr_t path, intptr_t flags, intptr_t notused);

int sys_close (int fd);
intptr_t _sys_close (intptr_t fd, intptr_t notused1, intptr_t notused2);

int sys_read (int fd, void *buffer, int len);
intptr_t _sys_read (intptr_t fd, intptr_t buffer, intptr_t len);

int sys_write (int fd, const void *buffer, int len);
intptr_t _sys_write (intptr_t fd, intptr_t buffer, intptr_t len);

int sys_read_timeout (int fd, int msec);
intptr_t _sys_read_timeout (intptr_t fd, intptr_t msec, intptr_t notused);

/** sysgetdent gets the next directory entry. Return value is
    1 for success, 0 when no more entries, and -errno on error. */
int sys_getdent (int fd, DirEntry *de);
intptr_t _sys_getdent (intptr_t fd, intptr_t de, intptr_t notused);

int sys_chdir (const char *path);
intptr_t _sys_chdir (intptr_t path, intptr_t notused1, intptr_t notused2);

int sys_getcwd (char *path, int size);
intptr_t _sys_getcwd (intptr_t path, intptr_t size, intptr_t notused);

int sys_fstat (int fd, struct stat *sb);
intptr_t _sys_fstat (intptr_t fd, intptr_t sb, intptr_t notused);

int sys_mkdir (const char *path);
intptr_t _sys_mkdir (intptr_t path, intptr_t notused1, intptr_t notused2);

int sys_poll_interrupt (int intr_num);
intptr_t _sys_poll_interrupt (intptr_t intr_num, intptr_t notused1, 
   intptr_t notused2);

int sys_clear_interrupt (int intr_num);
intptr_t _sys_clear_interrupt (intptr_t intr_num, intptr_t notused1, 
   intptr_t notused2);

int32_t sys_lseek (int fd, int32_t offset, int origin);
intptr_t _sys_lseek (intptr_t fd, intptr_t offset, intptr_t origin);

int sys_rename (const char *from_path, const char *to_path);
intptr_t _sys_rename (intptr_t from_path, intptr_t to_path, intptr_t unused);

int sys_unlink (const char *path);
intptr_t _sys_unlink (intptr_t path, intptr_t unused1, intptr_t unused2);

int sys_rmdir (const char *path);
intptr_t _sys_rmdir (intptr_t path, intptr_t unused1, intptr_t unused2);


struct tm *sys_gmtime_r (time_t t, struct tm *tm);
struct tm *sys_localtime_r (time_t t, struct tm *tm);
intptr_t _sys_gmtime_r (intptr_t t, intptr_t tm, intptr_t notused);
intptr_t _sys_localtime_r (intptr_t t, intptr_t tm, intptr_t notused);
intptr_t _sys_gettimeofday (intptr_t timeval, intptr_t timezone, intptr_t notused3);

int sys_utime (const char *path, const struct utimbuf *times);
intptr_t _sys_utime (intptr_t path, intptr_t times, intptr_t notused);

int sys_access (const char *path, int mode);
intptr_t _sys_access (intptr_t path, intptr_t mode, intptr_t notused);

intptr_t _sys_sbrk (intptr_t increment, intptr_t notused1, intptr_t notused2);
intptr_t _sys_exit (intptr_t status, intptr_t notused1, intptr_t notused2);

int sys_devctl (int fd, intptr_t arg1, intptr_t arg2);
intptr_t _sys_devctl (intptr_t fd, intptr_t arg1, intptr_t arg2);

intptr_t _sys_usleep (intptr_t usec, intptr_t notused1, intptr_t notused2);

intptr_t _sys_get_key (intptr_t fd_in, intptr_t notused2, intptr_t notused3);
intptr_t _sys_get_line (intptr_t fd_in, intptr_t buff, intptr_t len);

int32_t sys_ftruncate (int fd, int32_t len);
intptr_t _sys_ftruncate (intptr_t fd, intptr_t len, intptr_t notused);

#ifdef __cplusplus
}
#endif



