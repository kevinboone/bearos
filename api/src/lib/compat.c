/*===========================================================================
  api/src/lib/compat.c

  This file is part of the BearOS project. 

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
#include <errno.h>
#include <bearos/exec.h>	
#include <bearos/syscalls.h>	
#include <bearos/compat.h>	

/*===========================================================================
  Structs copied from bearos/sys. We don't want application programs to
  need to include platform files. However, we have to be careful to keep
  things in sync. 
===========================================================================*/
typedef struct _DirEntry
  {
  char name[256];
  int type;
  time_t mtime;
  int32_t size;
  } DirEntry;


extern int errno__;

/*===========================================================================
  getdelim
===========================================================================*/
ssize_t getdelim (char **buf, size_t *bufsiz, int delimiter, FILE *fp)
  {
  char *ptr, *eptr;

  if (*buf == NULL || *bufsiz == 0) {
	  *bufsiz = BUFSIZ;
	  if ((*buf = malloc(*bufsiz)) == NULL)
		  return -1;
  }

  for (ptr = *buf, eptr = *buf + *bufsiz;;) {
	  int c = fgetc(fp);
	  if (c == -1) {
		  if (feof(fp))
			  return ptr == *buf ? -1 : ptr - *buf;
		  else
			  return -1;
	  }
	  *ptr++ = c;
	  if (c == delimiter) {
		  *ptr = '\0';
		  return ptr - *buf;
	  }
	  if (ptr + 2 >= eptr) {
		  char *nbuf;
		  size_t nbufsiz = *bufsiz * 2;
		  ssize_t d = ptr - *buf;
		  if ((nbuf = realloc(*buf, nbufsiz)) == NULL)
			  return -1;
		  *buf = nbuf;
		  *bufsiz = nbufsiz;
		  eptr = nbuf + nbufsiz;
		  ptr = nbuf + d;
	  }
    }
  }

/*===========================================================================
  getline
===========================================================================*/
ssize_t getline(char **buf, size_t *bufsiz, FILE *fp)
  {
  return getdelim(buf, bufsiz, '\n', fp);
  }

/*===========================================================================
  opendir 
===========================================================================*/
DIR *opendir (const char *path)
  {
  DIR *self = 0;

  int fd = open (path, O_RDONLY);
  if (fd >= 0)
    {
    struct stat sb;
    fstat (fd, &sb);
    if ((sb.st_mode & S_IFMT) == S_IFDIR)
      {
      // OK -- if we get here, we've got a valid directory.
      // If we have to bomb now, ensure that fd gets closed.
      self = malloc (sizeof (DIR));
      if (self)
        {
        memset (self, 0, sizeof (DIR));
        self->fd = fd;
        errno__ = 0;
        }
      else
        {
        close (fd);
        errno__ = ENOMEM;
        }   
      self->fd = fd;
      }
    else
      errno__ = ENOTDIR;
    }
  else
    {
    // Do nothing -- errno will be set
    }
  
  return self;
  }

/*===========================================================================
  closedir 
===========================================================================*/
int closedir (DIR *self)
  {
  if (self->fd) close (self->fd);
  free (self);
  return 0;
  }

/*===========================================================================
  readdir 
===========================================================================*/
struct dirent *readdir (DIR *self)
  {
  DirEntry de;
  int r = syscall (BEAROS_SYSCALL_GETDENT, (int32_t)self->fd, 
      (int32_t)&de, 0); 

  if (r == 1) 
    {
    unsigned char d_type; 
    switch (de.type)
      {
      case S_IFBLK: d_type = DT_BLK; break;
      case S_IFCHR: d_type = DT_CHR; break;
      case S_IFDIR: d_type = DT_DIR; break;
      case S_IFIFO: d_type = DT_FIFO; break;
      case S_IFLNK: d_type = DT_LNK; break;
      case S_IFSOCK: d_type = DT_SOCK; break;
      default: d_type = DT_REG; 
      }
    strncpy (self->dirent.d_name, de.name, 256);
    self->dirent.d_type = d_type;
    return &(self->dirent);
    }

  if (r < 0) errno__ = -r;
  return 0;
  }

/*===========================================================================
  kbhit 
===========================================================================*/
int kbhit (void)
  {
  int c = read_timeout (1, 20);
  return (c != -1);
  }





