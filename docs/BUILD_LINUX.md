# Building a Linux version of BearOS

For testing purposes, it's possible to build a Linux version of BearOS, that
includes the basic kernel and shell. At present, there is no documented
way to build BearOS applications for the Linux version. However, testing
the shell on Linux is a good way to use tools like Valgrind, to check that
there are no memory leaks or careless pointer operations.

To build for Linux, you'll still need the Pico C SDK, and you'll still
need to build using CMake. The reason for this is to keep the number of
differences beween the Linux and Pico builds to the absolute minimum.

The basic process, once the Pico SDK is installed, is as follows:

    mkdir build_host
    cd build_host
    PICO_PLATFORM=host PICO_SDK_PATH=/path/to/sdk cmake ..
    make

In practice, you might need to provide additional CMake settings to indicate
the compiler to use.

The build process produces a binary called `bearos` which can be run
in the usual way. To exit the shell hit 'ctrl+D'. 

The Linux build of BearOS does not use the host filesytem, because it tried
to replicate the Pico version closely. It expects to find a FAT32 filesystem
image in a file `/tmp/fatfs_loopback.img`. To create this file:

    $ dd if=/dev/zero of=/tmp/fatfs_loopback.img count=1 bs=512M
    $ mkfs.vfat -F32 /tmp/fatfs_loopback.img

You can mount this `.img` file as a loopback device like this:

    $ sudo mount -o loop /tmp/fatfs_loopback.img /tmp

Running BearOS the filesytem image mounted in Linux is potentially
problematic, because there is no locking. However, it seems to be OK
for basic testing.



