/*============================================================================
 *  shell/shell_parser.c
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/
#define _GNU_SOURCE

/*============================================================================
 * ==========================================================================*/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <pico/stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/error.h>
#include <errno.h>
#include <shell/shell_parser.h>
#include <klib/string.h>
#include <klib/list.h>
#include <compat/compat.h>

/*============================================================================
 * Token methods 
 * ==========================================================================*/
/*============================================================================
 * token_create 
 * ==========================================================================*/
Token *token_create (TokenType type, const char *val, int line, int col)
  {
  Token *self = malloc (sizeof (Token));
  self->type = type;
  if (val) 
    self->val = strdup (val);
  else
    self->val = NULL; 
  self->line = line; self->col = col;
  return self;
  }

/*============================================================================
 * token_destroy
 * ==========================================================================*/
void token_destroy (Token *self)
  {
  if (self->val) free (self->val);
  free (self);
  }

/*============================================================================
 * token_dump
 * ==========================================================================*/
void token_dump (const Token *self)
  {
  compat_printf ("type=%d val=%s\n", self->type, self->val);
  }

/*============================================================================
 * Node methods 
 * ==========================================================================*/
/*=========================================================================
 * node_destroy
 * =======================================================================*/ 
void node_destroy (Node *self)
  {
  if (self->val1) free (self->val1);
  if (self->val2) free (self->val2);
  if (self->nodes) list_destroy (self->nodes);
  free (self);
  }

/*=========================================================================
 * node_create
 * =======================================================================*/ 
Node *node_create (int type)
  {
  Node *self = malloc (sizeof (Node));
  memset (self, 0, sizeof (Node));
  self->nodes = list_create ((ListItemFreeFn)node_destroy);
  self->type = type;
  return self;
  };

/*=========================================================================
 * node_dump 
 * =======================================================================*/ 
void node_dump (const Node *self, int level)
  {
  const char *name;
  switch (self->type)
    {
    case NODE_ARG: name = "arg"; break;
    case NODE_ARG_LIST: name = "arglist"; break;
    case NODE_REDIR: name = "redir"; break;
    case NODE_REDIR_LIST: name = "redirlist"; break;
    case NODE_ARGLIST_WITH_REDIRLIST: name = "arglist_with_redirlist"; break;
    case NODE_EXEC: name = "exec"; break;
    case NODE_ASSIGN: name = "assign"; break;
    case NODE_ASSIGNLIST: name = "assignlist"; break;
    case NODE_ASSIGNLIST_EXEC: name = "assignlist_exec"; break;
    case NODE_STATEMENT: name = "statement"; break;
    case NODE_PIPEDLIST: name = "pipedlist"; break;
    default: name = "?";
    }
  for (int i = 0; i < level; i++) printf (" ");
  printf ("%s val1=%s val2=%s\n", name, self->val1, self->val2);
  if (self->nodes)
    {
    int l = list_length (self->nodes);
    for (int i = 0; i < l; i++)
      {
      node_dump (list_get (self->nodes, i), level + 3);
      }
    }
  }

/*============================================================================
 * ShellParser methods 
 * ==========================================================================*/
/*============================================================================
 * Opaque struct 
 * ==========================================================================*/
struct _ShellParser
  {
  const List *tokens;
  int pos;
  int length;
  };

/*============================================================================
 * shellparser_new 
 * ==========================================================================*/
ShellParser *shellparser_new (const List *tokens)
  {
  ShellParser *self = malloc (sizeof (ShellParser));
  self->tokens = tokens;
  self->pos = 0;
  self->length = list_length (tokens);
  return self;
  }

/*============================================================================
 * shellparser_next
 * ==========================================================================*/
const Token *shellparser_next (ShellParser *self)
  {
  Token *t = list_get (self->tokens, self->pos);
  if (self->pos < self->length - 1)
    {
    self->pos++;
    }
  return t;
  }

/*============================================================================
 * shellparser_new 
 * ==========================================================================*/
void shellparser_destroy (ShellParser *self)
  {
  free (self);
  }

/*============================================================================
 * shellparser_arglist
 * arg_list => arg arg_list 
 * ==========================================================================*/
Node *shellparser_arglist (ShellParser *self)
  {
  int old_t = self->pos;

  const Token *t1 = shellparser_next (self);
  if (t1->type == TOK_ARG)
    {
    Node *n = node_create (NODE_ARG_LIST);

    BOOL is_arg;
    do
      {
      is_arg = FALSE;
      old_t = self->pos;
      Node *n1 = node_create (NODE_ARG);
      n1->val1 = strdup (t1->val);
      list_append (n->nodes, n1);

      t1 = shellparser_next (self);
      if (t1->type == TOK_ARG)
        is_arg = TRUE;

      } while (is_arg);
    self->pos = old_t;
    return n;
    }

  self->pos = old_t;
  return NULL;
  }

/*============================================================================
 * shellparser_redirlist
 * redir_list => redir redir_list 
 * ==========================================================================*/
Node *shellparser_redirlist (ShellParser *self)
  {
  int old_t = self->pos;

  const Token *t1 = shellparser_next (self);
  const Token *t2 = shellparser_next (self);

  if (t1->type == TOK_REDIR && t2->type == TOK_ARG)
    {
    Node *n = node_create (NODE_REDIR_LIST);

    BOOL is_redir;
    do
      {
      is_redir = FALSE;
      old_t = self->pos;
      Node *n1 = node_create (NODE_REDIR);
      n1->val1 = strdup (t1->val);
      n1->val2 = strdup (t2->val);
      list_append (n->nodes, n1);

      t1 = shellparser_next (self);
      t2 = shellparser_next (self);
      if (t1->type == TOK_REDIR && t2->type == TOK_ARG)
        is_redir = TRUE;

      } while (is_redir);
    self->pos = old_t;
    return n;
    }

  self->pos = old_t;
  return NULL;
  }

/*============================================================================
 * shellparser_assignlist
 * assinglist => assign assginlist 
 * ==========================================================================*/
Node *shellparser_assignlist (ShellParser *self)
  {
  int old_t = self->pos;

  const Token *t1 = shellparser_next (self);

  if (t1->type == TOK_ARG && strchr (t1->val, '='))
    {
    Node *n = node_create (NODE_ASSIGNLIST);
    BOOL is_assign;
    do
      {
      is_assign = FALSE;
      old_t = self->pos;
      Node *n1 = node_create (NODE_ASSIGN);
      n1->val1 = strdup (t1->val);
      list_append (n->nodes, n1);

      t1 = shellparser_next (self);
      if (t1->type == TOK_ARG && strchr (t1->val, '='))
	is_assign = TRUE;

      } while (is_assign);
    self->pos = old_t;
    return n;
    }

  self->pos = old_t;
  return NULL;
  }

/*=========================================================================
 * shellparser_arglist_with_redirlist
 * arglist_with_redirlist => arglist redirlist 
 * =======================================================================*/ 
Node *shellparser_arglist_with_redirlist (ShellParser *self)
  {
  int old_t = self->pos;

  Node *n = shellparser_arglist (self);
  if (n)
    {
    Node *n2 = shellparser_redirlist (self);
    if (n2)
      {
      Node *n3 = node_create (NODE_ARGLIST_WITH_REDIRLIST); 
      list_append (n3->nodes, n);
      list_append (n3->nodes, n2);
      return n3;
      }
    else 
      node_destroy (n);
    }

  self->pos = old_t;
  return NULL;
  }

/*=========================================================================
 * shellparser_exec
 * exec => arg_list_with_redir_list | arg_list 
 * =======================================================================*/ 
Node *shellparser_exec (ShellParser *self)
  {
  int old_t = self->pos;

  Node *n = shellparser_arglist_with_redirlist (self);
  if (n)
    {
    n->type = NODE_EXEC;
    return n;
    }

  self->pos = old_t;

  n = shellparser_arglist (self);
  if (n)
    {
    Node *n2 = node_create (NODE_EXEC);
    list_append (n2->nodes, n);
    return n2;
    }

  self->pos = old_t;
  return NULL;
  }

/*=========================================================================
 * shellparser_assignlist_exec
 * assignlist_exec => assignlist exec 
 * =======================================================================*/ 
Node *shellparser_assignlist_exec (ShellParser *self)
  {
  int old_t = self->pos;

  Node *n = shellparser_assignlist (self);
  if (n)
    {
    Node *n2 = shellparser_exec (self);
    if (n2)
      {
      Node *n3 = node_create (NODE_ASSIGNLIST_EXEC); 
      list_append (n3->nodes, n);
      list_append (n3->nodes, n2);
      return n3;
      }
    else 
      node_destroy (n);
    }

  self->pos = old_t;
  return NULL;
  }

/*=========================================================================
 * shellparser_statement
 * statement => assignlist_with_exec | assignlist | exec
 * =======================================================================*/ 
Node *shellparser_statement (ShellParser *self)
  {
  int old_t = self->pos;

  Node *n = shellparser_assignlist_exec (self);
  if (n)
    {
    Node *n2 = node_create (NODE_STATEMENT);
    list_append (n2->nodes, n);
    return n2;
    }

  self->pos = old_t;

  n = shellparser_assignlist (self);
  if (n)
    {
    Node *n2 = node_create (NODE_STATEMENT);
    list_append (n2->nodes, n);
    return n2;
    }

  n = shellparser_exec (self);
  if (n)
    {
    Node *n2 = node_create (NODE_STATEMENT);
    list_append (n2->nodes, n);
    return n2;
    }

  self->pos = old_t;
  return NULL;
  }


/*=========================================================================
 * shellparser_pipedlist
 * pipedlist => statement pipedlist
 * =======================================================================*/ 
Node *shellparser_pipedlist (ShellParser *self)
  {
  int old_t = self->pos;

  Node *n1 = shellparser_statement (self);

  if (n1)
    {
    Node *n2 = node_create (NODE_PIPEDLIST);
    
    BOOL is_statement;
    do
      {
      list_append (n2->nodes, n1);
      is_statement = FALSE;

      old_t = self->pos;
      const Token *t1 = shellparser_next (self);
      if (t1->type == TOK_PIPE)
        {
        n1 = shellparser_statement (self);
        if (n1)
          {
          is_statement = TRUE; 
          }
        else
          {
          free (n1);
          return n2;
          }
        }
      else
        {
        self->pos = old_t;
        return n2;
        }
      } while (is_statement);
    self->pos = old_t;
    return n1;
    }
  self->pos = old_t;
  return NULL;
  }

/*============================================================================
 * shellparser_parse_and_report
 * ==========================================================================*/
Node *shellparser_parse_and_report (ShellParser *self)
  {
  Node *n = shellparser_pipedlist (self); // TODO
  return n;
  }

/*============================================================================
 * shellparser_get_token_pos
 * ==========================================================================*/
int shellparser_get_token_pos (const ShellParser *self)
  {
  return self->pos;
  }

