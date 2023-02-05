/*============================================================================
 *  sys/filedesc.c
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/error.h>
#include <errno.h>
#include <sys/diskinfo.h>
#include <syslog/syslog.h>
#include <sys/fsmanager.h>
#include <sys/filedesc.h>

#define TRACE_IN SYSLOG_TRACE_IN
#define TRACE_OUT SYSLOG_TRACE_OUT
#define TRACE SYSLOG_TRACE
#define DEBUG SYSLOG_DEBUG
#define INFO SYSLOG_INFO
#define WARN SYSLOG_WARN

/*============================================================================
 * filedesc_new 
 * ==========================================================================*/
FileDesc *filedesc_new (void) 
  {
  TRACE_IN;
  FileDesc *self = malloc (sizeof (FileDesc));
  memset (self, 0, sizeof (FileDesc));

  TRACE_OUT;
  return self;
  }

/*============================================================================
 * filedesc_destroy
 * ==========================================================================*/
void filedesc_destroy (FileDesc *self) 
  {
  TRACE_IN;

  free (self);

  TRACE_OUT;
  }




