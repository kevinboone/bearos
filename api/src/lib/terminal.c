/*===========================================================================
  api/src/lib/term.c

  This file is part of the BearOS project. 

  Copyright (c)2022 Kevin Boone, GPL v3.0 

===========================================================================*/
#include <stddef.h>	
#include <stdio.h>	
#include <unistd.h>	
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>
#include <bearos/compat.h>	
#include <bearos/devctl.h>	
#include <bearos/syscalls.h>	
#include <bearos/terminal.h>	

#if BEAROS
//#include <bearos/printf.h>
//#include <bearos/compat.h>
//#include <bearos/devctl.h>
#endif


int oldflags;

/*===========================================================================
  terminal_get_key
===========================================================================*/
int terminal_get_key (int fd_in)
  {
  return syscall (BEAROS_SYSCALL_GET_KEY, fd_in, 0, 0);
  }

/*===========================================================================
  term_init
===========================================================================*/
int terminal_init (int fd_in, int fd_out)
  {
  (void)fd_out;
  // TODO -- what if term is not a TTY?
  devctl (fd_in, DC_TERM_GET_FLAGS, (int32_t)&oldflags);
  // TODO -- not a tty
  return 0;
  }

/*===========================================================================
  term_set_raw
===========================================================================*/
void terminal_set_raw (int fd_in)
  {
  devctl (fd_in, DC_TERM_SET_FLAGS, (int32_t)DC_TERM_FLAG_NOECHO);
  }

/*=========================================================================

  terminal_reset

=========================================================================*/
void terminal_reset (int fd_in, int fd_out)
  {
  (void)fd_out;
  devctl (fd_in, DC_TERM_SET_FLAGS, (int32_t)oldflags);
  }


/*=========================================================================

  terminal_get_props

=========================================================================*/
int terminal_get_props (int fd_in, DevCtlTermProps *props)
  {
  return devctl (fd_in, DC_TERM_GET_PROPS, (int32_t)props);
  }

/*=========================================================================

  terminal_get_line

=========================================================================*/
int terminal_get_line (int fd_in, char *buff, int len)
  {
  return syscall (BEAROS_SYSCALL_GET_LINE, (int32_t)fd_in, 
           (int32_t)buff, (int32_t)len);
  }



