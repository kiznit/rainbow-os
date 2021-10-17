Rainbow OS Toolchain
====================

This directory contains a CMake project to build the Rainbow OS toolchain.


Building the Rainbow OS toolchain
---------------------------------

Create a build folder. From the build folder, invoke CMake and specify an installation directory if desired:

    ```
    mkdir build
    cd build
    cmake -DCMAKE_INSTALL_PREFIX=~/opt/llvm ~/rainbow-os/toolchain
    make
    ```

You can of course use ninja to build the toolchain:

    ```
    mkdir build
    cd build
    cmake -GNinja -DCMAKE_INSTALL_PREFIX=~/opt/rainbow-os-toolchain ~/rainbow-os/toolchain
    ninja
    ```
