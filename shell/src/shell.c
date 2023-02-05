/*============================================================================
 *  shell/shell.c
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
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/error.h>
#include <errno.h>
#include <term/term.h>
#include <shell/shell.h>
#include <shell/shell_cmd.h>
#include <shell/shell_parser.h>
#include <klib/string.h>
#include <klib/list.h>
#include <sys/fsutil.h>
#include <sys/direntry.h>
#include <sys/syscalls.h>
#include <compat/compat.h>
#include <devmgr/devmgr.h>
#include <bearos/terminal.h>
#include <bearos/intr.h>

typedef struct _ShellParseContext 
  {
  int argc;
  char **argv;
  } ShellParseContext;

/*============================================================================
 * shell_poll_interrupt 
 * TODO
 * ==========================================================================*/
bool shell_poll_interrupt (void) 
  {
  return (sys_poll_interrupt (SYS_INTR_TERM) != 0);
  }

/*============================================================================
 * shell_clear_interrupt 
 * TODO
 * ==========================================================================*/
void shell_clear_interrupt (void) 
  {
  sys_clear_interrupt (SYS_INTR_TERM);
  }

/*============================================================================
 * prompt 
 * ==========================================================================*/
static void prompt ()
  {
  Process *p = process_get_current();
  const char *cwd = process_get_cwd (p);
  compat_printf ("%s>", cwd);
  }

/*=========================================================================
  shell_glob_match
=========================================================================*/
BOOL shell_glob_match (const char *filename, const char *match)
  {
  return !compat_fnmatch (match, filename, 0);
  }

/*=========================================================================
  shell_globber2
=========================================================================*/
static void shell_globber2 (String *token, List *list)
  {
  bool match = false;

  const char *_token = string_cstr (token);
  if (strchr (_token, '*') || strchr (_token, '?')) 
    {
    char basename[PATH_MAX];
    char dir[PATH_MAX];
    fsutil_get_basename (_token, basename, PATH_MAX);
    fsutil_get_dir (_token, dir, PATH_MAX);
    int fd;
    if (dir[0])
      fd = sys_open (dir, O_RDONLY);
    else
      fd = sys_open (".", O_RDONLY);
    if (fd >= 0)
      {
      DirEntry de;
      while (sys_getdent (fd, &de) == 1)
         {
         if (de.name[0] == '.') continue;
         if (shell_glob_match (de.name, basename)) 
           {
           char newpath[PATH_MAX];
           fsutil_join_path (dir, de.name, newpath, PATH_MAX);
           Token *t = token_create (TOK_ARG, newpath, 0, 0); // TODO
           list_append (list, t);
           match = true;
           }
         } 
      sys_close (fd);
      }
    }

  if (match)
    {
    }
  else
    {
    Token *t = token_create (TOK_ARG, string_cstr (token), 0, 0); // TODO
    list_append (list, t);
    }
  }


/*=========================================================================

  shell_do_variable
  A helper function for processing env tokens of the form 'foo=bar' in
    a specified environment. The slight complexity is that we need to
    remove a variable if no value is given ('foo='). So we can't just
    use environment_set_raw, which would be a lot faster.

=========================================================================*/
static Error shell_do_variable (const char *env_tok, Environment *env)
  {
  int ret = 0;
  // We don't need to check whether the = sign is present -- this function
  //   would not be called if it wasn't.
  char *s_ = strdup (env_tok);
  char *eqpos = strchr (s_, '='); 
  char *value = eqpos + 1; // May be pointing to the final 0 
  *eqpos = 0;
  char *name = s_; 
  if (value[0])
    environment_set (env, name, value, TRUE);
  else
    environment_delete (env, name);
  free (s_);
  return ret;
  }

/*=========================================================================

  shell_add_variable

=========================================================================*/
Error shell_add_variable (const char *env_tok)
  {
  Process *p = process_get_current();
  Environment *env = p->environment;
  return shell_do_variable (env_tok, env);
  }

/*=========================================================================

  shell_var_lookup

=========================================================================*/
const char *shell_var_lookup (const char *var, void *context)
  {
  ShellParseContext *p = context;

  int arg = atoi(var);
  if (arg >= 1 || strcmp (var, "0") == 0)
    {
    if (arg >= p->argc) return "";
    return p->argv[arg];
    }
  else
    {
    const char *result = process_getenv (process_get_current(), var);
    if (result) return result;
    }
  return "";
  }

/*=========================================================================
  shell_exec_exec
=========================================================================*/
static Error shell_exec_exec  (const Node *exec, int _argc, char **_argv,
          const char *redir_in, const char *redir_out)
  {
  Error ret = 0;

  const Node *redirs = NULL;
  const Node *arglist = list_get (exec->nodes, 0);
  if (list_length (exec->nodes) >= 2)
    redirs = list_get (exec->nodes, 1);
  
  //printf ("ARGLIST\n");
  //node_dump (arglist, 0);

  BOOL redirs_ok = TRUE;

  int fd_stdout = -1;
  int fd_stdin = -1;
  Process *p = process_get_current();
  FileDesc *old_stdout_filedesc = NULL;
  FileDesc *old_stdin_filedesc = NULL;

  if (redir_in)
    {
    fd_stdin = sys_open (redir_in, O_RDONLY);
    if (fd_stdin >= 0)
      {
      old_stdin_filedesc = p->files[STDIN_FILENO];
      p->files[STDIN_FILENO] = p->files[fd_stdin]; 
      }
    else
      redirs_ok = FALSE;
    }

  if (redir_out)
    {
    fd_stdout = sys_open (redir_out, O_WRONLY | O_TRUNC);
    if (fd_stdout >= 0)
      {
      old_stdout_filedesc = p->files[STDOUT_FILENO];
      p->files[STDOUT_FILENO] = p->files[fd_stdout]; 
      }
    else
      redirs_ok = FALSE;
    }

  if (redirs)
    {
    int n_redirs = list_length (redirs->nodes);
    for (int i = 0; i < n_redirs; i++)
      {
      const Node *n = list_get (redirs->nodes, i);
      const char *redir = n->val1;
      const char *filename = n->val2;
      if (strcmp (redir, ">") == 0)
        {
        if (fd_stdout >= 0)
          {
          compat_printf_stderr ("Ignoring second output redirection\n");
          continue;
          }
        fd_stdout = sys_open (filename, O_WRONLY | O_TRUNC);
        if (fd_stdout >= 0)
          {
          old_stdout_filedesc = p->files[STDOUT_FILENO];
          p->files[STDOUT_FILENO] = p->files[fd_stdout]; 
          }
        else
          {
          ret = -fd_stdout;
          compat_printf ("Can't redirect to %s: %s\n", 
             filename, strerror (ret));
          redirs_ok = FALSE;
          }
        }
      else if (strcmp (redir, ">>") == 0)
        {
        if (fd_stdout >= 0)
          {
          compat_printf_stderr ("Ignoring second output redirection\n");
          continue;
          }
        fd_stdout = sys_open (filename, O_WRONLY | O_APPEND);
        if (fd_stdout >= 0)
          {
          old_stdout_filedesc = p->files[STDOUT_FILENO];
          p->files[STDOUT_FILENO] = p->files[fd_stdout]; 
          }
        else
          {
          ret = -fd_stdout;
          compat_printf ("Can't redirect to %s: %s\n", 
             filename, strerror (ret));
          redirs_ok = FALSE;
          }
        }
      else if (strcmp (redir, "<") == 0)
        {
        if (fd_stdin >= 0)
          {
          compat_printf_stderr ("Ignoring second input redirection\n");
          continue;
          }
        fd_stdin = sys_open (filename, O_RDONLY);
        if (fd_stdin >= 0)
          {
          old_stdin_filedesc = p->files[STDIN_FILENO];
          p->files[STDIN_FILENO] = p->files[fd_stdin]; 
          }
        else
          {
          ret = -fd_stdin;
          compat_printf ("Can't redirect from %s: %s\n", 
             filename, strerror (ret));
          redirs_ok = FALSE;
          }
        }
      }
    }

  if (redirs_ok)
    {
    int argc = list_length (arglist->nodes);
    char **argv = malloc ((size_t)(argc + 1) * sizeof (char *));

    for (int i = 0; i < argc; i++)
      {
      Node *n = list_get (arglist->nodes, i);
      argv[i] = strdup (n->val1);
      }
    argv[argc] = NULL;

    ret = shell_cmd (argc, argv);

    for (int i = 0; i < argc; i++)
      {
      free (argv[i]);
      }
    free (argv);
    }

  if (redirs || redir_in || redir_out)
    {
    if (fd_stdout >= 0) 
      {
      p->files[STDOUT_FILENO] = old_stdout_filedesc;
      sys_close (fd_stdout);
      }
    if (fd_stdin >= 0) 
      {
      p->files[STDIN_FILENO] = old_stdin_filedesc;
      sys_close (fd_stdin);
      }
    }

  return ret;
  }

/*=========================================================================
  shell_assignlist_to_env
  Process a NODE_ASSIGNLIST, adding variables found there to the 
    specified environment.
=========================================================================*/
static void shell_assignlist_to_env (const Node *assignlist, Environment *env)
  {
  int n_assigns = list_length (assignlist->nodes);
  for (int i = 0; i < n_assigns; i++)
    {
    const Node *n = list_get (assignlist->nodes, i);
    const char *env_token = n->val1;
    shell_do_variable (env_token, env);
    } 
  }

/*=========================================================================
  shell_exec_assignlist_exec
  Execute shell_exec_exec() in a new environment, that contains the
    env settings in the assignlist. Then restore the old environment.
=========================================================================*/
static Error shell_exec_assignlist_exec (const Node *assignlist_exec, 
         int _argc, char **_argv, 
         const char *redir_in, const char *redir_out)
  {
  int ret;
  const Node *assignlist = list_get (assignlist_exec->nodes, 0);
  const Node *exec = list_get (assignlist_exec->nodes, 1);

  Process *p = process_get_current();
  Environment *old_env = p->environment;

  Environment *new_env = environment_clone (old_env);
  shell_assignlist_to_env (assignlist, new_env);

  p->environment = new_env;
  ret = shell_exec_exec (exec, _argc, _argv, redir_in, redir_out);
  p->environment = old_env;
  environment_destroy (new_env);
  return ret;
  }

/*=========================================================================
  shell_exec_statement
=========================================================================*/
static Error shell_exec_statement (const Node *n, int argc, char **argv, 
         const char *redir_in, const char *redir_out)
  {
  Error ret = 0;
  switch (n->type)
    {
    case NODE_STATEMENT:
      Node *n_child = list_get (n->nodes, 0);
      switch (n_child->type)
        {
        case NODE_EXEC:
	  ret = shell_exec_exec (n_child, argc, argv, redir_in, redir_out);
          break;
        case NODE_ASSIGNLIST_EXEC:
	  ret = shell_exec_assignlist_exec (n_child, 
            argc, argv, redir_in, redir_out);
          break;
        case NODE_ASSIGNLIST:
          // When we have an assignlist on its own (without a following
          //   thing to execute), we set the environment of the current
          //   process. With assignlist_exec, we must add the assignlist
          //   to a new environment, and add it to the current process.
          //   So env changes with assignlist_exec are transient.
          Process *p = process_get_current();
          shell_assignlist_to_env (n_child, p->environment);
          break;
        default:
          printf ("Error\n");
        }
      break;
    default:
      compat_printf ("Internal error: unexpected top of statement tree\n");
      ret = EINVAL;
    }
  return ret;
  }

/*=========================================================================
  shell_exec_pipedlist
=========================================================================*/
static Error shell_exec_pipedlist (const Node *n, int argc, char **argv)
  {
  Error ret = 0;
  switch (n->type)
    {
    case NODE_PIPEDLIST:
      //node_dump (n, 0);
      int len = list_length (n->nodes);
      if (len == 1)
        {
        Node *statement = list_get (n->nodes, 0);
        ret = shell_exec_statement (statement, argc, argv, 
                NULL, NULL);
        }
      else
        {
        /*
        This is how the tempfiles are circulated between stages of the pipe:
        0 < stdin > tmpfile_a
        1 < tmpfile_a > tmpfile_b
        2 < tmpfile_b > tmpfile_a
        3 < tmpfile_a > stdout
        */
        Process *p = process_get_current();
        const char *tmp = process_getenv (p, "TMP");
        char tmpfile_a[64], tmpfile_b[64];
        snprintf (tmpfile_a, sizeof (tmpfile_a), "%s/000-redir-a", tmp);
        snprintf (tmpfile_b, sizeof (tmpfile_a), "%s/000-redir-b", tmp);
        char *redir_in, *redir_out;
        for (int i = 0 ; i < len; i++)
          {
          if (i == 0)
            {
            redir_in = NULL;
            redir_out = tmpfile_a;
            }
          else if (i == len - 1)
            {
            if (i % 2 == 0)
              {
              redir_in = tmpfile_b;
              }
            else
              {
              redir_in = tmpfile_a;
              }
            redir_out = NULL;
            }
          else 
            {
            if (i % 2 == 0)
              {
              redir_in = tmpfile_b;
              redir_out = tmpfile_a;
              }
            else
              {
              redir_in = tmpfile_a;
              redir_out = tmpfile_b;
              }
            }

          Node *statement = list_get (n->nodes, i);
          ret = shell_exec_statement (statement, argc, argv, 
                  redir_in, redir_out);
          // TODO -- handle ret
          } 
        sys_unlink (tmpfile_a);
        sys_unlink (tmpfile_b);
        }
      break;
    default:
      compat_printf ("Internal error: unexpected top of statement tree\n");
      ret = EINVAL;
    }
  return ret;
  }

/*=========================================================================
  shell_do_buffer
  argc and argv here are args passed to a shell script. If this is a 
    single line from the CLI, argc will be zero.
  The file argument is only used for error reporting.
=========================================================================*/
Error shell_do_buffer (const char *buff, const char *file, 
        int argc, char **argv)
  {
  String *sbuff = string_create (buff);
  string_tok_globber = shell_globber2;
  string_tok_var_lookup = shell_var_lookup;

  ShellParseContext spc;
  spc.argc = argc;
  spc.argv = argv;

  int ret = 0;
  List *args = string_tokenize2 (sbuff, &spc); 
  int len = list_length (args);
  if (len > 1) // The token list will always end with an EOI token.
    {
    /*
    int l = list_length (args);
    for (int i = 0; i < l; i++)
      {
      Token *t = (Token *)list_get (args, i);
      printf ("i=%d type=%d val=%s line=%d col=%d\n", i, t->type, t->val, 
	t->line, t->col);
      }
    */

    ShellParser *sp = shellparser_new (args);
    Node *n = shellparser_parse_and_report (sp);

    if (n)
      {
      ret = shell_exec_pipedlist (n, argc, argv);
      node_destroy (n);
      }
    else
      {
      // TODO -- find a way to report errors properly
      int p = shellparser_get_token_pos (sp);
      Token *t = list_get (args, p);
      int line = t->line;
      int col = t->col;
      printf ("Syntax error, file '%s': line %d, col %d\n", 
        file ? file : "stdin",  line + 1, col + 1);
      ret = EINVAL;
      }

    shellparser_destroy (sp);
    }

  list_destroy (args);
  string_destroy (sbuff);
  return ret;
  }

/*=========================================================================

  shell_do_line

=========================================================================*/
Error shell_do_line (const char *buff, int argc, char **argv)
  { 
  //Error ret = shell_do_line_fragment (buff);
  Error ret = shell_do_buffer (buff, "console", argc, argv);
  return ret;
  }


/*============================================================================
 * shell_run
 * ==========================================================================*/
void shell_run (void)
  {
  char line [SHELL_MAX_LINE];
  List *history = list_create (free);

  while (prompt (), term_get_line (0, /* TODO */ line, sizeof (line), 
        10, history) != TGL_EOI) 
    {
    if (line[0])
      {
      shell_do_line (line, 0, NULL);
      }
    }
  list_destroy (history);
  }

/*============================================================================
 * shell_process
 * ==========================================================================*/
Error shell_process (int argc, char **argv, char **envp)
  {
  (void)argc;
  (void)argv;
  (void)envp;
  sys_mkdir ("A:/tmp"); // We should probably check the env var
  shell_run ();
  return 0;
  }

