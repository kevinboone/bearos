/*============================================================================
 *  syslog/syslog.h
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

#pragma once

/*============================================================================
 * ==========================================================================*/

typedef int SyslogLevel;

#define LOGLEVEL_TRACE 4
#define LOGLEVEL_DEBUG 3 
#define LOGLEVEL_INFO  2 
#define LOGLEVEL_WARN  1 
#define LOGLEVEL_ERROR 0

#define SYSLOG_TRACE(...)  syslog_trace ( __FUNCTION__, __LINE__, __VA_ARGS__)
#define SYSLOG_DEBUG(...)  syslog_debug ( __FUNCTION__, __LINE__, __VA_ARGS__)
#define SYSLOG_INFO(...)  syslog_info ( __FUNCTION__, __LINE__, __VA_ARGS__)
#define SYSLOG_WARN(...)  syslog_info ( __FUNCTION__, __LINE__, __VA_ARGS__)
#define SYSLOG_ERROR(...)  syslog_info ( __FUNCTION__, __LINE__, __VA_ARGS__)

#define SYSLOG_TRACE_IN SYSLOG_TRACE("in")
#define SYSLOG_TRACE_OUT SYSLOG_TRACE("out")

#ifdef __cplusplus
extern "C" {
#endif

void syslog (SyslogLevel level, const char *msg, ...);
void syslog_trace (const char *func, int line, const char *msg, ...);
void syslog_debug (const char *func, int line, const char *msg, ...);
void syslog_info (const char *func, int line, const char *msg, ...);
void syslog_warn (const char *func, int line, const char *msg, ...);
void syslog_error (const char *func, int line, const char *msg, ...);

extern void syslog_set_level (SyslogLevel _level);

#ifdef __cplusplus
}
#endif




