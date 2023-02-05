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
#include <setjmp.h>
#include <sys/stat.h>
#include <sys/syscalls.h>
#include <sys/error.h>
#include <errno.h>
#include <sys/diskinfo.h>
#include <sys/fsutil.h>
#include <sys/syscall.h>
#include <syslog/syslog.h>
#include <sys/process.h>
#include <sys/environment.h>
#include <sys/elf.h>
#include <bearos/exec.h>
#include <compat/compat.h>

#define TRACE_IN SYSLOG_TRACE_IN
#define TRACE_OUT SYSLOG_TRACE_OUT
#define TRACE SYSLOG_TRACE
#define DEBUG SYSLOG_DEBUG
#define INFO SYSLOG_INFO
#define WARN SYSLOG_WARN

Process *current = 0;

/*===========================================================================
 * process_new 
 * =========================================================================*/
Process *process_new (void) 
  {
  TRACE_IN;
  Process *self = malloc (sizeof (Process));
  memset (self, 0, sizeof (Process));
  strcpy (self->cwd, "A:/");
  self->environment = environment_new();
  TRACE_OUT;
  return self;
  }

/*===========================================================================
 * process_new_clone
 * =========================================================================*/
extern Process *process_new_clone (const Process *old)
  {
  Process *self = process_new();
  strcpy (self->cwd, old->cwd);

  Environment *new_env = self->environment;
  char **old_envp = process_get_envp ((Process *)old); 
  while (*old_envp)
    {
    environment_add_raw (new_env, *old_envp);
    old_envp++;
    }

  return self;
  }

/*============================================================================
 * process_destroy
 * ==========================================================================*/
void process_destroy (Process *self) 
  {
  TRACE_IN;

  for (int i = 0; i < NFILES; i++)
    { 
    if (self->files[i]) filedesc_destroy (self->files[i]);
    }

  if (self->environment) 
    environment_destroy (self->environment);

  free (self);

  TRACE_OUT;
  }

/*============================================================================
 * process_destroy
 * ==========================================================================*/
Error process_run (Process *self, EntryFn fn, int argc, 
                      char **argv)
  {
  TRACE_IN;
  Process *old_process = process_set_current (self);
  int ret;
  int r = setjmp (self->exit_jmp);
  if (r == 0)
    {
    char **envp = process_get_envp (self);
    ret = fn (argc, argv, envp);
    }
  else
    ret = r;

  process_set_current (old_process);
  TRACE_OUT;
  return ret;
  }
  
/*============================================================================
 * process_close_all_files
   Close all files that are still open when process completes.
 * ==========================================================================*/
void process_close_all_files (Process *self)
  {
  for (int i = 0; i < NFILES; i++)
    {
    FileDesc *f = self->files[i];
    if (f) 
      {
      sys_close (i);
      self->files[i] = NULL;
      }
    }
  }

/*============================================================================
 * process_set_filedesc
 * ==========================================================================*/
Error process_set_filedesc (Process *self, int n, FileDesc *filedesc)
  {
  TRACE_IN;
  // TODO check params and limits
  self->files[n] = filedesc;
  TRACE_OUT;
  return 0;
  }

/*============================================================================
 * process_release_filedesc
 * ==========================================================================*/
Error process_release_filedesc (Process *self, int n)
  {
  TRACE_IN;
  DEBUG ("Releas FD %d", n);
  if (self->files[n])
    {
    filedesc_destroy (self->files[n]);
    }
  else
    {
    DEBUG ("FD %d was not allocated", n);
    }
  self->files[n] = 0;
  TRACE_OUT;
  return 0;
  }

/*============================================================================
 * process_get_current
 * ==========================================================================*/
Process *process_get_current (void)
  {
  return current;
  }

/*============================================================================
 * process_set_current
 * ==========================================================================*/
Process *process_set_current (Process *p)
  {
  TRACE_IN;
#ifdef DEBUG
  DEBUG ("Setting process %p", p);
#endif
  Process *old = current;
  current = p;
  TRACE_OUT;
  return old;
  }

/*============================================================================
 * process_get_next_fd
 * ==========================================================================*/
Error process_get_next_fd (const Process *self, int *fd) 
  {
  TRACE_IN;
  for (int i = 0; i < NFILES; i++)
    {
    if (self->files[i] == 0)
      {
      *fd = i;
      return 0;
      }
    }
  TRACE_OUT;
  return ENFILE;
  }

/*============================================================================
 * process_get_filedesc
 * ==========================================================================*/
Error process_get_filedesc (Process *self, int fd, FileDesc **filedesc)
  {
  if (fd < 0 || fd >= NFILES)
    {
    WARN ("Invalid fd: %d", fd);
    return EINVAL;
    }

  *filedesc = self->files[fd];
  return 0;
  }

/*============================================================================
 * process_get_current_drive
 * ==========================================================================*/
int8_t process_get_current_drive (const Process *self)
  {
  return (int8_t)(toupper (self->cwd[0]) - 'A');
  }

/*============================================================================
 * process_get
 * ==========================================================================*/
const char *process_get_cwd (const Process *self)
  {
  return self->cwd;
  }

/*============================================================================
 * process_set_cwd
 * ==========================================================================*/
extern Error process_set_cwd (Process *self, const char *cwd)
  {
  int fd = sys_open (cwd, O_RDONLY);
  if (fd >= 0) 
    {
    Error ret;
    if (self->files[fd]->type == S_IFDIR)
      {
      char abspath[PATH_MAX];
      fsutil_make_abs_path (cwd, abspath, PATH_MAX);
      strncpy (self->cwd, abspath, PATH_MAX); 
      ret = 0;
      } 
    else
      ret = ENOTDIR;
    sys_close (fd);
    return ret;
    }
  else
    {
    return -fd;
    }
  }

/*============================================================================
 * process_get_utc_offset
 * ==========================================================================*/
int process_get_utc_offset (const Process *self)
  {
  const char *utc = process_getenv (self, "UTC_OFFSET");
  if (utc) return atoi (utc);
  return 0;
  }

/*============================================================================
 * process_getenv
 * ==========================================================================*/
const char *process_getenv (const Process *self, const char *name)
  {
  if (self->environment == 0) return 0;
  return environment_get (self->environment, name);
  }

/*============================================================================
 * process_setenv
 * ==========================================================================*/
void process_setenv (Process *self, const char *name, const char *value)
  {
  if (value[0])
    environment_set (self->environment, name, value, true);
  else
    environment_delete (self->environment, name);
  }

/*============================================================================
 * process_set_break
 * ==========================================================================*/
void process_set_break (Process *self, void *brk)
  {
  // printf ("set break to %p\n", brk);
  self->brk = brk;
  }

/*============================================================================
 * process_get_break
 * ==========================================================================*/
void *process_get_break (const Process *self)
  {
  // printf ("Get break  %p\n", self->brk);
  return self->brk;
  }

/*============================================================================
 * process_get_envp
 * ==========================================================================*/
char **process_get_envp (Process *self)
  {
  return environment_get_envp (self->environment);
  }


/*============================================================================
 * process_run_file
 * ==========================================================================*/
Error process_run_file (Process *self, const char *path, int argc,
                         char **argv)
  {
  // TODO: What do we do with the environment here?
  Error ret = 0;
  if (argc >= 1)
    {
    const char *filename = path;
    ret = elf_check (filename);
    if (ret == 0)
      {
      struct stat sb;
      int fd = sys_open (filename, O_RDONLY);
      sys_fstat (fd, &sb); // Should succeed by this point
  //printf ("size = %d\n", sb.st_size);
  //printf ("bytes to read = %d\n", to_read);
      
      int32_t pos = sys_lseek (fd, ELF_TEXT_OFFSET, SEEK_SET);
      if (pos == ELF_TEXT_OFFSET)
	{

#if PICO_ON_DEVICE
        int to_read = (int)sb.st_size - (int)ELF_TEXT_OFFSET;
        int prog_size = to_read;
        // We can't do _any_ of this stuff on the host!
	//printf ("Seek OK");
	char *load_pos = (char *)BEAROS_LOAD_ADDRESS;
	while (to_read > 0)
	  {
	  char buff[256];
	  sys_read (fd, buff, 256);
	  //printf ("pos = %p\n", load_pos);
	  memcpy (load_pos, buff, sizeof (buff));
	  load_pos += (int)sizeof (buff);
	  to_read -= (int)sizeof (buff);
	  }

        sys_close (fd);

        int32_t *syscall_addr = (int32_t *)BEAROS_SYSCALL_VECTOR; 
        *syscall_addr = (int32_t)syscall;

        process_set_break (self, 
          (void *)(BEAROS_LOAD_ADDRESS + prog_size + 1024));

	EntryFn entry = (EntryFn)(BEAROS_LOAD_ADDRESS | 0x01);

        ret = process_run (self, entry, argc, argv);
#endif
	}
      else
        {
        sys_close (fd);
	compat_printf ("Seek error: %s\n", strerror (-pos));
	}
      }
    else
      {
      ret = ENOEXEC;
      compat_printf ("Bad ELF file: %s\n", filename);
      }
    }
  else
    {
    // This should never happen
    compat_printf ("usage: %s {filename}\n", argv[0]);
    ret = EINVAL;
    }
  return ret;
  }

/*============================================================================
 * process_open_file
 * ==========================================================================*/
Error process_open_file (Process *self, int fd, const char *file, int flags)
  {
  Error ret = 0;
  Process *old = process_set_current (self);

  int fd2 = sys_open (file, flags);
  if (fd2 >= 0)
    {
    // Better hope there isn't a file descriptor in this slot already
    // Also, it's possible that, when this method is called, there are
    //   no existing open files. So the newly-assigned fd might equal
    //   the fd we want to assign to. So we need to take care of that
    //   situation.
    self->files[fd] = self->files[fd2];
    if (fd != fd2)
      self->files[fd2] = NULL;
    }
  else
    ret = -fd2;

  process_set_current (old);
  return ret;
  }


