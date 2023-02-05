/*============================================================================
 *  shell/shell_parser.h
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

#pragma once

/*============================================================================
 * ==========================================================================*/

#include <stdint.h>
#include <sys/error.h>
#include <klib/list.h>
#include <sys/process.h>

/*========================================================================
 * Node types
 * ======================================================================*/

// NODE_ARG is a general string. It has no child nodes, and val1 is the
//   string text.
#define NODE_ARG 1

// NODE_ARG_LIST is a list of NODE_ARGS. val1 = val2 = NULL, and each
//   child node is a node of type NODE_ARG
#define NODE_ARG_LIST 2

// NODE_REDIR is a redirection specification. val1 is the redirection
//   operator (<<, >, etc), and val2 is the filename to which to 
//   redirect
#define NODE_REDIR 3

// NODE_REDIR_LIST is a list of redirections, typically following a
//   command/argument list. Each child node is a node of type NODE_REDIR.
#define NODE_REDIR_LIST 4

// NODE_ARGLIST_WITH_REDIRLIST represents a command/arg list followed
//   by a list of redirections. It has two child nodes; the first is
//   a node of type NODE_ARG_LIST, which in turn contains the command and
//   arguments. The second is of type NODE_REDIR_LIST, which contains
//   a list of redirections.
#define NODE_ARGLIST_WITH_REDIRLIST 5

// NODE_EXEC is the basic unit of execution. It is a command/argument
//   group with optional redirections. It has one o two child nodes. The
//   first, which will always be present, is a NODE_ARG_LIST containing
//   the command and arguments. The second, which may or may not be
//   present, contains the redirections, in the form of a NODE_REDIR_LIST.
#define NODE_EXEC 6

// NODE_ASSIGN contains a single assignment expression ("foo=bar") in its val1
//   attribute
#define NODE_ASSIGN 7

// NODE_ASSIGNLIST contains one or more MODE_ASSIGN nodes in its nodelist
#define NODE_ASSIGNLIST 8

// NODE_ASSIGNLIST_EXEC contains a NODE_ASSIGNLIST as its first child node,
//   then a NODE_EXEC in the second child node
#define NODE_ASSIGNLIST_EXEC 9

// NODE_STATEMENT contains a single child node. It will be of type 
//   NODE_ASSIGNLIST_EXEC or NODE_ASSIGNLIST or NODE_EXEC
#define NODE_STATEMENT 10

// NODE_PIPEDLIST is a list of NODE_STATEMENT, separated by | characters.
//   There may be only one item.
#define NODE_PIPEDLIST 11

typedef enum TokenType
  {
  TOK_EOI, TOK_ARG, TOK_REDIR, TOK_SEMI, TOK_PIPE
  } TokenType;

typedef struct _Token
  {
  char *val;
  TokenType type;
  int line; 
  int col;
  } Token;

typedef struct _Node
  {
  int type;
  char *val1;
  char *val2;
  List *nodes;
  } Node;

struct ShellParser;
typedef struct _ShellParser ShellParser;

#ifdef __cplusplus
extern "C" {
#endif

extern void node_destroy (Node *self);

extern Token *token_create (TokenType type, const char *val, 
          int line, int col);
extern void token_destroy (Token *self);

extern ShellParser *shellparser_new (const List *tokens);
extern void shellparser_destroy (ShellParser *self);
extern Node *shellparser_parse_and_report (ShellParser *self);

// Get the number of the token where the parser stopped. If this isn't
//   the number of the EOI token after a parse, there was an error.
extern int shellparser_get_token_pos (const ShellParser *self);

#ifdef __cplusplus
}
#endif



