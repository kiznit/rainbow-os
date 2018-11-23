Rainbow OS
==========

Thierry's own OS.


License
=======

Rainbow is licensed under the BSD (Simplified) 2-Clause License.


Required tools
--------------

To build Rainbow from your existing operating system, the following tools are
required (version numbers show what I am using and do not indicate strict
requirements):

* binutils 2.26.1
* gcc 6.5.0
* GNU Make 4.1

And finally if you want to run your disk image under an emulator, I use qemu:

* qemu-system-i386 2.6.50
* qemu-system-x86_64 2.6.50

Note that at this time I am NOT using cross-compilers. I am using the tools that
come with my Linux Mint 18.3 installation. Yon can, of course, use a cross-compiler.


Supported configurations
------------------------

| Machine | Firmware | Architectures |
|---------|----------|---------------|
| PC      | uefi     | ia32, x86_64  |


Build
-----

There is a top level Makefile in the root of the project that will automatically
create the build directory, build the subprojects and generate a bootable image.

1) Building with defaults (using the host's toolchain)

    ```
    make
    ```

2) Building for a different architecture (using the host's toolchain)

    ```
    make ARCH=ia32
    make ARCH=x86_64
    ```

3) Building using a cross-compiler (in which case `ARCH` will be ignored and determined by calling the cross compiler)

    ```
    make CROSS_COMPILE=ia32-elf-
    make CROSS_COMPILE=x86_64-elf-
    ```

4) Create an bootable image

    ```
    make image
    ```

5) Run the image in the emulator

    ```
    make run
    ```
