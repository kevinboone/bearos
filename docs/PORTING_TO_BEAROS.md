## Building or porting C programs for use with BearOS

C programs can be built for BearOS on Linux, and maybe on other platforms.  It
is designed to use the same ARM toolchain that is recommended for the Pi Pico
SDK, that is `arm-none-eabi-gcc` etc., with the ARM Newlib standard library. If
you have installed the Pico SDK, then you have everything needed to build for
BearOS.

### Principle

The priniples of building for BearOS are set out here:

    https://kevinboone.me/pico_run_ram2.html

Essentially, you need to build an ELF binary for the ARM thumb instruction set,
whose program load address is the value defined in `api/bearos/bearos/exec.h`.
The C files in `/api/src/lib` contain the Newlib stubs, that is, the interfaces
between Newlib and the Bearos kernel, and a few other things that might be
helpful.

The C source in `/api/src/start` is the startup code, which must be compiled
and loaded at the program's load address. Typically, that means that it must be
the first object file seen by the linker.

The output from the linker must be one large text segment followed by one BSS
segment. The BSS segment will be zeroed by the start-up code, so it must define
specific symbols to indicate its start and end. A suitable linker script is in
`/api/link/script.ld`.

It is a feature of the GCC linker that the executable code will start at a
position in the output file that matches the least-significant word of the load
address. That is, if the load address is 0x20005000, the program code will
start at 0x5000 in the output file. The BearOS program loader relies on this
being the case, and avoids the complexity of an ELF parser.  If this stops
being an assuption that can be relied on, BearOS will need a more sophisticated
loader.

There are quite a few example programs that demonstrate the principles of
building code for BearOS. The trick is to compress the generated binaries as
much as possible, to remove any unnecessary information. BearOS will load the
entire ELF binary into memory, except for the initial offset, so some trouble
has to be taken with this compression. The example programs use a combination
of `strip` and `objcopy` to post-process the binary.

## Headers

The BearOS API has been designed so that it is possible to use the standard GCC
headers, like `stdio.h` and `stat.h`. So, for example, the BearOS `stat()`
function takes a standard Unix-like `struct stat` as an argument.  The main
fields of the structure are filled in as C programmer might expect, but there
are differences. BearOS has no concept of an inode, for example, so data fields
related to inodes will not be meaninful. But the `st_mode` field of `struct
stat` takes the same numeric values for type: `S_IFDIR` for a directory, for
example, The 'mode' parts of this field are always zero, because BearOS has no
concept of file permissions.

It's hard to document all the similarities and differences between the BearOS
API and standard C. 

There are further complexities, also. For example `opendir` takes an argument
of type `DIR` while `readdir` returns a `struct dirent` as usual.  This is all
as a programmer might expect, but the ARM GCC compiler does not provide any
implemntation of necessary data structures. In fact, using 

    # include <dirent.h>

will cause a fatal compilation error. So these structures are defined in
`api/bearos/bearos/compat.h`, which a program can include.

## stdin, stdout, and stderr

BearOS implements these concepts, and programs can read from the conventional
file descriptor 0 for input, and write to 1 and 2. The shell will be able to
redirect these data streams as a Unix shell does.

## Terminal issues

It's very helpful to be able to build Linux and BearOS versions of programs
from the same source, but complications arise because BearOS expects its output
device to be a dumb terminal. A way to manage this complexity is to set the
terminal up to emulate a Linux virtual console, but some terminals cannot be
set up this way. Broadly, BearOS is designed to run with a terminal whose
characteristics are defined in the file `TERMINAL.md`. The Linux virtual
console is not a good match for these settings. Consequently, it's usually
necessary, when developing a program that does complicated terminal
input/output, to write separate sections for BearOS and Linux.

## Environment

The environment maintained by the BearOS shell is passed to the programs it
loads in the usual way, so functions like `getenv()` and `putenv()` work.
Programmers should be aware, however, that any data stored in the environment
goes into the kernel's memory area, not the program's, and this memory is a
very scarce resource. This behaviour might be changed in future, if it turns
out to be a significant problem. 

## Memory allocation

BearOS programs can use `malloc()` and related functions in the usual way. The
dynamic memory allocator assumes that it has control over all the memory from
the top of the program's binary, up to the stack at the top of RAM. By default,
BearOS allows 4kB for the stack. There's a risk, as there is on other
platforms, that the stack can get large enough to reach down to the dynamic
memory area, and this will cause catastrophic results. Programs need to be
careful about using large amounts of heap and large amounts of recursion at the
same time.

This is true on Linux as well, but Linux systems usually have hugely more
memory available.

A program can ignore the memory allocator completely, if it has the smarts.
The program can call `sbrk(0)` to find the top of its binary in RAM, and then
use any memory between this point and the stack (at 0x20040000) as it sees fit. 

## GPIO

See the file `GPIO.md` for an idea how a program may do rudimentary GPIO
operations using the `gpio` character device. Of course, a program can
interract directly with the GPIO hardware if necessary.

## Gotchas

Programs must not close stdin or stdout, even if they are redirected.  File
descriptor duplication is not implemented and, when the shell launches a
process, the file descriptors are simply copied. The descriptors seen by the
shell and the process point to the same objects in memory. If the program
closes stdin or stdout, this directly affects the file/device references held
by the shell, which will probably crash.  This problem requires a proper
implementation of fd dup'ing to fix.

The Newlib library, even in its 'Nano' variety, is not very space-efficient.  A
`Hello, world` program that uses `printf()` will be about 30kB in size.  This
is trivial by modern standards, but BearOS only has about 200kB available in
total. Part of this problem stems from the interdependencies between different
parts of Newlib. For example, `print()` has a dependency on `malloc()`, which
means that the entire memory allocator has to be included in the build.
Avoiding the standard library entirely is the way to get compact builds, but
this is not usually practicable. There are alternative implementations of
`printf()` and related functions that are much more memory-efficient than the
Newlib version; but these generally don't do any buffering, so additional code
will be needed to made them fast enough to be useful. 

The `open()` function works as usual, except that the third argument, file
mode, is ignored. BearOS has no concept of file permissions.

BearOS programs do not need to free all their memory, or close all their files,
before exit. The BearOS kernel will take care of this. However, given how
little memory is available, any program with a resource leak of this kind is
likely to misbehave long before it finishes. 

`spawn()`, `exec()`, `system()`, etc -- these do not work, and never will.
BearOS is unashamedly a single-tasking system. In principle, we could implement
`system()` so that one program could load another, and wait for it to complete
-- but there's just not enough RAM to make the additional complexity
worthwhile.

Similarly, although the Pico has two CPU cores, BearOS uses only one, and is
not designed ever to do differently. The additional locking and synchronization
that would be needed in the kernel, to make it thread safe, is just not worth
the trouble. You could use the second code for things completely separate from
BearOS, of course.

BearOS pathnames using Unix-like file separators, but DOS-like drive letters.
Since at present only one drive is supported, the drive letters can mostly be
ignored. However, it's worth bearing in mind that library functions that
operate on Unix-like paths will probably fail if there are, in fact, drive
letters present. The function 'getcwd()', for example, returns a full pathname
with a drive letter.

