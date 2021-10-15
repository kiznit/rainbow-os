Rainbow OS
==========

Thierry's own OS.


License
=======

Rainbow is licensed under the BSD (Simplified) 2-Clause License.


Required tools
--------------

To build Rainbow OS, you will need a recent version of clang (C17), clang++ (C++20), lld and CMake.

I use:

* clang 12.0.1
* clang++ 12.0.1
* lld 12.0.1
* cmake 3.21


If you want to run your disk image under an emulator, I use QEMU:

* qemu-system-aarch64 version 4.2.1
* qemu-system-x86_64 version 4.2.1

To run unit tests:

* C++20 compiler + standard library (I use clang 12)


Installing tools (Linux Mint 20.1)
----------------------------------

* sudo apt-get install qemu-system-aarch64
* sudo apt-get install qemu-system-x86


Supported configurations
------------------------

| Description | Machine | Architectures |
|-------------|---------|---------------|
| ARM 64-bits | uefi    |   aarch64     |
| PC          | uefi    |   x86_64      |



Build
-----

There is a top level CMakeLists.txt in the root of the project that should be used from a build directory.
This should be used to build the bootloader, kernel and any other required subprojects.

1) Configuring rainbow-os with default options:

    ```
    mkdir build
    cd build
    cmake ~/rainbow-os
    ```

2) Configuring rainbow-os for a specific architecture

    ```
    cmake -DARCH=aarch64 ~/rainbow-os
    cmake -DARCH=aarch64 ~/rainbow-os
    ```

3) Building the bootloader, kernel and subprojects:

    ```
    make
    ```

4) Create a bootable FAT32 disk image:

    ```
    make image
    ```

5) Run the image in the default emulator

    ```
    make run
    ```

6) Build and run unit tests

    ```
    make check
    ```

7) Configure build to generate code coverage
    ```
    cmake -DCMAKE_BUILD_TYPE=Debug -DCODE_COVERAGE=ON ~/rainbow-os
    make ccov
    ```
