Rainbow OS
==========

Thierry's own OS.


License
=======

Rainbow is licensed under the BSD (Simplified) 2-Clause License.


Building Rainbow
================

Required tools
--------------

To build Rainbow from your existing operating system, the following tools are
required (version numbers show what I am using and do not indicate strict
requirements):

    1) binutils 2.27 (cross-platform tools)
    2) gcc 6.3.0 (cross-platform compilers)
    3) MinGW-w64 (i686-w64-mingw32 and x86_64-w64-mingw32 cross-compilers)
    5) GNU Make 3.81
    6) Python 3.4.3 (for configure script)

If you are unsure how to produce the required binutils and gcc cross-compilers,
take a look here: http://wiki.osdev.org/GCC_Cross-Compiler.

There is also an easy to use Makefile here: https://github.com/kiznit/build-gcc-and-binutils

To generate an ISO image of Rainbow, you will also need:

    6) grub-mkrescue 2.02

And finally if you want to run your iso image under an emulator, I use qemu:

    7) qemu-system-xxx 2.6.50


Build
-----

1) Create a directory where you want the build to happen. I simply use 'build':

    mkdir build

2) Run the configure script. This will generate a Makefile in the current directory.

3) make image

4) make run
