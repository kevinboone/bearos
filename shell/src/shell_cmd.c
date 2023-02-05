/*============================================================================
 *  shell/shell_cmd.c
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/

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
#include <klib/string.h>
#include <klib/list.h>
#include <sys/fsutil.h>
#include <bearos/devctl.h>
#include <sys/direntry.h>
#include <sys/syscalls.h>
#include <sys/clocks.h>
#include <compat/compat.h>
#include <devmgr/devmgr.h>
#include <gfx/fonts.h>

#include <pico/types.h>
#include <waveshare_lcd/waveshare_lcd.h>

//extern WSLCD *global_wslcd; // TODO
//#include <hardware/rtc.h>
//#include <ds3231/ds3231.h>
Error shell_cmd_foo (int argc, char **argv)
  {
int year, month, day, hour, min, sec;
  ds3231_get_datetime (ds3231_inst, &year, &month, &day, &hour, &min, &sec); 
printf ("YMS=%d %d %d\n", year, month, day);
  return 0;
  }

/*============================================================================
 * ==========================================================================*/
static struct Cmd
  {
  const char *name;
  ShellCmdFn fn;
  } cmd_table [] =  
  {
  {"cat", shell_cmd_cat},
  {"cd", shell_cmd_cd},
  {"clear", shell_cmd_clear},
  {"cp", shell_cmd_cp},
  {"date", shell_cmd_date},
  {"df", shell_cmd_df},
  {"env", shell_cmd_env},
  {"gpio", shell_cmd_gpio},
  {"grep", shell_cmd_grep},
  {"ls", shell_cmd_ls},
  {"echo", shell_cmd_echo},
  {"mkdir", shell_cmd_mkdir},
  {"mv", shell_cmd_mv},
  {"rm", shell_cmd_rm},
  {"rmdir", shell_cmd_rmdir},
  {"source", shell_cmd_source},
  {"uname", shell_cmd_uname},
  {"foo", shell_cmd_foo},
  {0, 0}
  };

/*============================================================================
 * shell_try_path
 * ==========================================================================*/
Error shell_try_path (const char *path, int argc, char **argv)
  {
  Error ret = ENOENT;
  if (sys_access (path, X_OK) == 0)
    {
    if (fsutil_is_regular (path))
      {
      ret = shell_cmd_run_file (path, argc, argv);;
      }
    }
  return ret;
  }

/*============================================================================
 * shell_do_external
 * ==========================================================================*/
Error shell_do_external (int argc, char **argv)
  {
  Error ret = shell_try_path (argv[0], argc, argv);
  if (ret == ENOENT)
    {
    const char *path = process_getenv (process_get_current(), "PATH");
    if (path)
      {
      char *path2 = strdup (path);
      char *tok = strtok (path2, ";");
      while (tok && ret == ENOENT)
        {
        char try[PATH_MAX];
        fsutil_join_path (tok, argv[0], try, PATH_MAX); 
        ret = shell_try_path (try, argc, argv);
        tok = strtok (NULL, ";");
        }
      free (path2);
      }
    }
  return ret;
  }

/*============================================================================
 * shell_cmd
 * ==========================================================================*/
Error shell_cmd (int argc, char **argv)
  {
  Error ret = shell_do_external (argc, argv);
  if (ret != ENOENT) return ret;
 
  int i = 0;
  struct Cmd *p = &(cmd_table[0]);
  do
    {
    if (strcmp (argv[0], p->name) == 0)
      {
      Error ret = p->fn (argc, argv);
      return ret;
      }

    i++;
    p = &(cmd_table[i]);
    } while (p->name);

  compat_printf ("%s: bad command\n", argv[0]);
  return ENOENT;
  }

