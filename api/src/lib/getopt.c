/*===========================================================================
  api/src/lib/lib.c

  This file is part of the BearOS project. 

  Copyright (c)2022 Kevin Boone, GPL v3.0 

===========================================================================*/
#include <getopt.h>

#ifndef BEAROS_NO_GETOPT

char *optarg;
int optind = 1, optopt, __optpos, optreset = 0;

#define optpos __optpos

int getopt (int argc, char * const argv[], const char *optstring)
  {
  int i, c, d;
  int k, l;

  if (!optind || optreset) 
    {
    optreset = 0;
    __optpos = 0;
    optind = 1;
    }

  if (optind >= argc || !argv[optind])
    return -1;

  if (argv[optind][0] != '-') 
    {
    if (optstring[0] == '-') 
      {
      optarg = argv[optind++];
      return 1;
      }
    return -1;
    }

  if (!argv[optind][1])
    return -1;

  if (argv[optind][1] == '-' && !argv[optind][2])
    return optind++, -1;

  if (!optpos) optpos++;
  c = argv[optind][optpos], k = 1;
  optopt = c;
  optpos += k;

  if (!argv[optind][optpos]) 
    {
    optind++;
    optpos = 0;
    }

  if (optstring[0] == '-' || optstring[0] == '+')
    optstring++;

  i = 0;
  d = 0;
  do 
    {
    d = optstring[i], l = 1;
    if (l>0) i+=l; else i++;
    } while (l && d != c);

  if (d != c) 
    {
    return '?';
    }

  if (optstring[i] == ':') 
    {
    if (optstring[i+1] == ':') optarg = 0;
    else if (optind >= argc) 
      {
      if (optstring[0] == ':') return ':';
      return '?';
      }
    if (optstring[i+1] != ':' || optpos) 
      {
      optarg = argv[optind++] + optpos;
      optpos = 0;
      }
    }
  return c;
  }

#endif

