Rainbow OS
==========

Thierry's own OS.


License
=======

Rainbow is licensed under the BSD (Simplified) 2-Clause License.


Required tools
--------------

The first step is to install build-essential:

```
sudo apt-get install build-essential
```

To build Rainbow OS, you will need a recent version of clang (C17 and C++20) and CMake.

* cmake 3.28.3
* clang 18.1.3
* lld 18.1.3
* llvm-ar 18.1.3
* llvm-ranlib 18.1.3
* mtools 4.0.43
* dosfstools 4.2

To install the above softtware, I used these commands:

```
sudo apt install cmake
sudo apt install clang
sudo apt install lld
sudo apt install llvm
sudo apt install mtools
sudo apt install dosfstools
```

If you want to run your disk image under an emulator, I use QEMU:

* qemu-system-x86_64 version 8.2.2
* qemu-system-aarch64 version 8.2.2

To install the above software, I used these commands:

```
sudo apt install qemu-system-x86
sudo apt install qemu-system-arm
```


To run unit tests:

* C++20 compiler + standard library


Supported configurations
------------------------

| Description       | Machine | Architectures |
|-------------------|---------|---------------|
| Generic aarch64   | generic |   aarch64     |
| Raspberry Pi 3    | raspi3  |   aarch64     |
| PC                | generic |   x86_64      |


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
```

3) Configuring rainbow-os for a specific machine:

```
cmake -DMACHINE=raspi3 ~/rainbow-os
```


4) Building the bootloader, kernel and subprojects:

```
make
```

5) Create a bootable FAT32 disk image:

```
make image
```

6) Run the image in the default emulator

```
make run
```

7) Build unit tests

```
make unittests
```

8) Build and run unit tests

```
make check
```

9) Configure build to generate code coverage

```
cmake -DCMAKE_BUILD_TYPE=Debug -DCODE_COVERAGE=ON ~/rainbow-os
make coverage
```
