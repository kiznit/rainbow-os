Rainbow OS
==========

Thierry's own OS.


License
=======

Rainbow is licensed under the BSD (Simplified) 2-Clause License.


Tools
=====

Quick guide to setuping the required tools:

```
sudo apt install clang
sudo apt install cmake
sudo apt install lld
sudo apt install mtools
sudo apt install dosfstools
```

Optional tools
--------------

```
sudo apt install clang-format
sudo apt install ninja
sudo apt install qemu-system-x86
sudo apt install qemu-system-arm
```

Build
=====

There is a top level CMakeLists.txt in the root of the project that should be used from a build directory.
This should be used to build the bootloader, kernel and any other required subprojects.

First create a build directory (I use "build" under the rainbow-os root folder):

```
mkdir build
cd build
```

Supported configurations
------------------------

| Description       | Machine | Architectures |
|-------------------|---------|---------------|
| Generic aarch64   | generic |   aarch64     |
| Generic x86_64    | generic |   x86_64      |


Configure
---------

1) Configuring rainbow-os with default options:

```
cmake <path to rainbow-os root>
```

1) Configuring rainbow-os for a specific architecture

```
cmake -DARCH=aarch64 <path to rainbow-os root>
```

Build
-----

These commands assume you are in the build folder.

1) Create a bootable FAT32 disk image:

```
make image
```

1) Run the image in the default emulator

```
make run
```
