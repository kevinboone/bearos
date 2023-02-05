SECTIONS
{
.text : ALIGN(0x04) { *.o(.text .rodata .data) }
.bss :
  {
  __bss_start__ = .;
  *(.bss*)
  *(COMMON)
  __bss_end__ = .;
  }
}

