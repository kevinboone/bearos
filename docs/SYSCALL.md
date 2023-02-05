# Syscalls in BearOS

Application code calls BearOS kernel functions (I/O, timing, GPIO) using a
syscall mechanism that is broadly the same as that of a real operating system.
At present, there is a single kernel entry point, whose address is stored at
memory location 0x20003F00. Note -- that's not the entry point, it's the
address of the entry point. The entry point itself changes every time the
BearOS kernel is built. The entry point address is defined in `bearos/exec.h`
as `BEAROS_SYSCALL_VECTOR`. The entry point should be entered by making
an ARM `bx` call _in thumb mode_. That is, the least-significant bit
of the address shoult be set, even though the address is an odd number.

Every kernel syscall (at present) takes a function number, and up to three
arguments. Standard ARM ABI methods are used for the argument passing. Each
argument is 32-bits in size, which is large enough for a pointer as well as an
integer. No system calls at present take arguments that are other than 32-bit
integers or pointer.

The syscall function numbers are defined in `bearos/syscalls` but application
programs should never have to use them. Applications should use the standard C
library, and functions in `api/lib/lib.c` make the syscalls in response.

The arguments to the syscalls differ, of course, in type and number. This
complexity is hidden when using the C library. 


