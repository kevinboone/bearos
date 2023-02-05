/*============================================================================
 * Start-up file for BearOS programs. 
 * This start-up is intended to be used when the program is linked with
 *   the newlib-nano C library. It assumes that __bss_start_ and __bss_end__
 *   are appropriately defined as the start and ends of the BSS section,
 *   which is the job of the linker script.
 *
 * Copyright (c)2022 Kevin Boone
============================================================================*/

extern int main (int argc, char **argv);
void *_sbrk (int increment);
char **environ;
extern int __bss_start__, __bss_end__;

unsigned int start (int argc, char **argv, char **envp)
  {

  // Blank the BSS section.
  for (char *p = (char*)&__bss_start__; p < (char*)&__bss_end__; p++) *p = 0;

  // initialize the memory allocator. In theory, we should need to set
  //   __malloc_free_list to null, since it's in the BSS section. To get
  //   the base address for allocation, we called sbrk(0), which returns
  //   the current program break, which will have been set by the
  //   program loader.
  extern void *__malloc_free_list;
  extern void *__malloc_sbrk_start;
  __malloc_free_list = 0;
  __malloc_sbrk_start = _sbrk (0);

  // The program launcher will pass the environment as the third argument.
  // The newlib setenv/getenv expect this pointer to be stored in a 
  //   variable 'environ'.
  // Note -- since this variable is likely to be in BSS, we have to 
  //   set it after zeroing BSS
  environ = envp;
  
  // Just call main.
  return main (argc, argv);
  }

