/*============================================================================
 *  sys/environment.c
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/
#define _GNU_SOURCE
#include <stdint.h>
#include <stdlib.h>
#include <pico/stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/syscalls.h>
#include <sys/error.h>
#include <errno.h>
#include <sys/environment.h>

/*============================================================================
 * Opaque structure 
 * ==========================================================================*/
struct _Environment
  {
  char **envp;
  };

/*============================================================================
 * environment_new 
 * ==========================================================================*/
Environment *environment_new (void) 
  {
  Environment *self = malloc (sizeof (Environment));
  memset (self, 0, sizeof (Environment));
  self->envp = malloc (1 * sizeof (char *));
  self->envp[0] = NULL;
  return self;
  }

/*============================================================================
 * environment_clone
 * size does not include the final null pointer
 * ==========================================================================*/
Environment *environment_clone (const Environment *old)
  {
  Environment *self = environment_new ();
  int l = environment_size (old);

  for (int i = 0; i < l; i++)
    {
    const char *token = old->envp[i];
    environment_add_raw (self, token);
    }

  return self;
  }

/*============================================================================
 * environment_size
 * size does not include the final null pointer
 * ==========================================================================*/
int environment_size (const Environment *self)
  { 
  int i = 0;
  while (self->envp[i]) 
    {
    i++;
    }
  return i;
  }

/*============================================================================
 * environment_add
 * ==========================================================================*/
Error environment_add (Environment *self, const char *name, 
    const char *value) 
  {
  int i = environment_size (self);
  self->envp = realloc (self->envp, (size_t)((i + 2) * (int)sizeof (char *)));
  char *s;
  asprintf (&s, "%s=%s", name, value);
  self->envp[i] = s;
  self->envp[i + 1] = 0; 
  return 0;
  }

/*============================================================================
 * environment_add_raw
 * ==========================================================================*/
Error environment_add_raw (Environment *self, const char *token)
  {
  int i = environment_size (self);
  self->envp = realloc (self->envp, (size_t)((i + 2) * (int)sizeof (char *)));
  self->envp[i] = strdup (token);
  self->envp[i + 1] = 0; 
  return 0;
  }

/*============================================================================
 * environment_destroy
 * ==========================================================================*/
void environment_destroy (Environment *self) 
  {
  if (self->envp)
    {
    int i = 0;
    while (self->envp[i]) 
      {
      free (self->envp[i]);
      i++;
      }
    free (self->envp);
    }
  free (self);
  }

/*============================================================================
 * environment_get_item
 * ==========================================================================*/
static int environment_get_item (const Environment *self, const char *name)
  {
  int i = 0;
  const char *s;
  while ((s = self->envp[i])) 
    {
    char *eqpos = strchr (s, '=');    
    if (eqpos)
      {
      if (strncmp (name, s, (size_t)(eqpos - s)) == 0) return i;
      }
    i++;
    }
  return -1;
  }

/*============================================================================
 * environment_dump
 * ==========================================================================*/
static void environment_dump (const Environment *self)
  {
  char **envp = self->envp;
  int i = 0;
  while (envp[i])
    {
    printf ("%d %s\n", i, envp[i]);
    i++;
    }
  }

/*============================================================================
 * environment_delete_item
 * ==========================================================================*/
static void environment_delete_item (Environment *self, int item)
  {
  if (item < 0) return;

  // We run the risk of freeing and moving memory outside envp here. 
  // We can't really check that 'item' is in bounds without enumerating
  //   the whole env array, which would be slow.
  char *to = self->envp[item];
  free (to);

  do
    {
    self->envp[item] = self->envp[item + 1];
    item++;
    } while (self->envp[item]);

  }

/*============================================================================
 * environment_get
 * ==========================================================================*/
const char *environment_get (const Environment *self, const char *name)
  {
  int i = 0;
  const char *s;
  while ((s = self->envp[i])) 
    {
    char *eqpos = strchr (s, '=');    
    if (eqpos)
      {
      if (strncmp (name, s, (size_t)(eqpos - s)) == 0) return eqpos + 1;
      }
    i++;
    }
  return 0;
  }

/*============================================================================
 * environment_delete
 * ==========================================================================*/
void environment_delete (Environment *self, const char *name)
  {
  int i = environment_get_item (self, name);
  if (i >= 0)
    {
    environment_delete_item (self, i);
    }
  }


/*============================================================================
 * environment_set
 * ==========================================================================*/
Error environment_set (Environment *self, const char *name, const char *value,
       bool overwrite)
  {
  char *exist = (char *)environment_get (self, name);
  if (exist)
    {
    if (overwrite)
      environment_delete (self, name);
    else
      return EINVAL;
    }
  environment_add (self, name, value);
  return 0;
  }

/*============================================================================
 * environment_get_envp
 * ==========================================================================*/
char **environment_get_envp (const Environment *self)
  {
  return self->envp;
  }


