/*============================================================================
 *  bearos/compat.h
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/
#pragma once

#include <stdio.h>
#include <sys/types.h>

#define DT_REG 0
#define DT_BLK 1
#define DT_CHR 2
#define DT_FIFO 3
#define DT_LNK 4
#define DT_SOCK 5
#define DT_DIR 6 

struct dirent
  {
  unsigned char d_type;
  char d_name[256];
  };

typedef struct DIR
  {
  int fd;
  struct dirent dirent;
  } DIR;

#ifdef __cplusplus
extern "C" {
#endif

extern ssize_t getline (char **lineptr, size_t *n, FILE *stream);
extern int read_timeout(int fd, int msec);
extern int32_t syscall (int32_t num, int32_t arg1, int32_t arg2, int32_t arg3);

DIR *opendir (const char *path);
int closedir (DIR *self);
struct dirent *readdir (DIR *self);

int kbhit (void);

#ifdef __cplusplus
}
#endif



