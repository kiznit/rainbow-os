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

    1) binutils 2.24.90 (i686-elf and x86_64-elf tools)
    2) gcc 4.8.3 (i686-elf and x86_64-elf cross-compilers)
    3) MinGW-w64 (i686-w64-mingw32 and x86_64-w64-mingw32 cross-compilers)
    4) NASM 2.10.09
    5) GNU Make 3.81

If you are unsure how to produce the required binutils and gcc cross-compilers,
take a look here: http://wiki.osdev.org/GCC_Cross-Compiler.

To generate an ISO image of Rainbow, you will also need:

    6) grub-mkrescue 2.02

And finally if you want to run your iso image under an emulator, I use qemu:

    7) qemu-system-x86_64 2.0.0


Makefile targets
----------------

    | Command          | Description                                    |
    |------------------|------------------------------------------------|
    | make             | Build all kernel variants                      |
    | make clean       | Cleanup                                        |
    | make bios-image  | Build a BIOS image (.iso file)                 |
    | make efi-image   | Build an EFI FAT32 image (.img file)           |
    | make run         | Run the kernel under qemu (BIOS)               |
    | make run-bios    | Run the kernel under qemu (BIOS)               |
    | make run-bios-32 | Run the kernel under qemu (BIOS, 32 bits CPU)  |
    | make run-bios-64 | Run the kernel under qemu (BIOS, 64 bits CPU)  |
    | make run-efi     | Run the kernel under qemu (EFI)                |
    | make run-efi-32  | Run the kernel under qemu (EFI, 32 bits CPU)   |
    | make run-efi-64  | run the kernel under qemu (EFI, 64 bits CPU)   |
    | make run-bochs   | Run the kernel under bochs (BIOS)              |
    | make run-tests   | Run unit tests                                 |
