Rainbow OS Toolchain
====================

This directory contains a makefile to build GCC as a cross-compiler.


Required tools
--------------

A modern version of GNU Make is required. I use GNU Make 4.1.


Available targets
-----------------

| Makefile target     | Description |
|---------------------|-------------|
| i686-rainbow-elf    | 32-bit x86  |
| x86_64-rainbow-elf  | 64-bit x86  |


Building the Rainbow cross-compiler
-----------------------------------

Simply invoke "make" with your desired target.

For example, if you want to build the x86_64 cross-compiler, you would use:

    make x86_64-rainbow-elf
