Rainbow OS
==========

Thierry's own OS.


License
=======

Rainbow is licensed under the BSD (Simplified) 2-Clause License.


Required tools
--------------

The first step is to build the Rainbow cross-compiler for your platform. This can easily be done by invoking "make" in the "toolchain" folder.
See https://github.com/kiznit/rainbow-os/tree/master/toolchain for more details.

Grub is required to generate a BIOS image:

* grub-mkrescue 2.02
* grub-pc 2.02
* xorriso 1.4.8 (Grub dependency required to manipulate ISO images)

And finally if you want to run your disk image under an emulator, I use qemu and bochs:

* qemu-system-i386 2.11.1
* qemu-system-x86_64 2.11.1
* Bochs x86 Emulator 2.6


Installing tools (Linux Mint 19.1)
----------------------------------

* sudo apt-get update
* sudo apt-get install build-essential
* sudo apt-get install xorriso
* sudo apt-get install qemu-system-x86
* sudo apt-get install bochs bochs-sdl


Supported configurations
------------------------

| Description | Machine | Architectures |
|-------------|----------|---------------|
| PC          | bios     | ia32, x86_64  |
| PC          | uefi     | ia32, x86_64  |



Build
-----

There is a top level Makefile in the root of the project that will automatically
create the build directory, build the subprojects and generate a bootable image.

1) Building with defaults

    ```
    make
    ```

2) Building for a different architecture

    ```
    make ARCH=ia32
    make ARCH=x86_64
    ```

3) Building using a specific cross-compiler (in which case `ARCH` will default to what the cross compiler targets)

    ```
    make CROSS_COMPILE=i686-elf-
    make CROSS_COMPILE=x86_64-elf-
    ```

4) Building for specific machine

    ```
    make MACHINE=bios
    make MACHINE=efi
    ```

5) Create an bootable image

    ```
    make image
    ```

6) Run the image in the default emulator

    ```
    make run
    ```

7) Run the image in a specific emulator

    ```
    make run-bochs
    make run-qemu
    ```

8) You can of course combine the above incantations

    ```
    make ARCH=ia32 run
    make ARCH=ia32 MACHINE=bios run-bochs
    ```
