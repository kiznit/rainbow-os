# Copyright (c) 2021, Thierry Tremblay
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

cmake_minimum_required(VERSION 3.21)
project(RainbowOS VERSION 0.0.1)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(code-coverage)


###############################################################################
#
# Configuration
#
################################################################################

set(VALID_ARCH aarch64 x86_64)

if (NOT DEFINED ARCH)
    set(ARCH ${CMAKE_SYSTEM_PROCESSOR})
    message("ARCH set to ${ARCH}")
else()
    message("ARCH is ${ARCH}")
endif()

if (NOT ARCH IN_LIST VALID_ARCH)
    message(FATAL_ERROR "ARCH must be one of ${VALID_ARCH}")
endif()

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
    message("CMAKE_BUILD_TYPE set to ${CMAKE_BUILD_TYPE}")
else()
    message("CMAKE_BUILD_TYPE is ${CMAKE_BUILD_TYPE}")
endif()


if (CMAKE_GENERATOR MATCHES "Ninja")
    set(GENERATOR_COMMAND ninja)
else()
    set(GENERATOR_COMMAND make)
endif()


if (NOT DEFINED CODE_COVERAGE)
    set(CODE_COVERAGE OFF)
    message("CODE_COVERAGE set to ${CODE_COVERAGE}")
else()
    message("CODE_COVERAGE is ${CODE_COVERAGE}")
endif()


###############################################################################
#
# Bootloader and kernel require a specific toolchain and cannot be built
# using the same compiler used for other Rainbow-OS components. Unfortunately
# cmake cannot handle multiple toolchains from within the same project. We use
# the usual workaround of building external projects to handle this situation.
#
################################################################################

include(ExternalProject)

ExternalProject_Add(
    external_boot
    BUILD_ALWAYS 1
    PREFIX ${CMAKE_BINARY_DIR}/boot
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/boot
    BINARY_DIR ${CMAKE_BINARY_DIR}/boot
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_TOOLCHAIN_FILE=${CMAKE_SOURCE_DIR}/cmake/toolchains/${ARCH}-none-uefi.cmake -B ${CMAKE_BINARY_DIR}/boot
    BUILD_COMMAND ${GENERATOR_COMMAND}
    INSTALL_COMMAND ""
)

ExternalProject_Add(
    external_kernel
    BUILD_ALWAYS 1
    PREFIX ${CMAKE_BINARY_DIR}/kernel
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/kernel
    BINARY_DIR ${CMAKE_BINARY_DIR}/kernel
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_TOOLCHAIN_FILE=${CMAKE_SOURCE_DIR}/cmake/toolchains/${ARCH}-none-elf.cmake -B ${CMAKE_BINARY_DIR}/kernel
    BUILD_COMMAND ${GENERATOR_COMMAND}
    INSTALL_COMMAND ""
)


###############################################################################
#
# Unit tests
#
###############################################################################

enable_testing()

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND})

if (CODE_COVERAGE)
    add_coverage(
        DEPENDS check
        SRCDIR ${CMAKE_SOURCE_DIR}
        BINDIR ${CMAKE_BINARY_DIR}
        EXCLUDE ${CMAKE_SOURCE_DIR}/third_party/* ${CMAKE_SOURCE_DIR}/unittest/*
    )
endif()

add_subdirectory(boot)
add_subdirectory(kernel)
add_subdirectory(metal)
add_subdirectory(unittest)


###############################################################################
#
# UEFI disk image
#
###############################################################################

add_custom_target(image DEPENDS external_boot external_kernel)


add_custom_command(
    TARGET image
    POST_BUILD
	COMMENT "Building UEFI disk image"
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/image/efi/boot
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/boot/src/*.efi ${CMAKE_BINARY_DIR}/image/efi/boot/
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/image/efi/rainbow
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/boot/src/*.efi ${CMAKE_BINARY_DIR}/image/efi/rainbow/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/kernel/src/kernel ${CMAKE_BINARY_DIR}/image/efi/rainbow/
    COMMAND dd if=/dev/zero of=${CMAKE_BINARY_DIR}/rainbow-efi-${ARCH}.img bs=1M count=33
    COMMAND mkfs.vfat ${CMAKE_BINARY_DIR}/rainbow-efi-${ARCH}.img -F32
    COMMAND mcopy -s -i ${CMAKE_BINARY_DIR}/rainbow-efi-${ARCH}.img ${CMAKE_BINARY_DIR}/image/* ::
)


###############################################################################
#
# Firmware (for emulation)
#
###############################################################################

if (ARCH STREQUAL "x86_64")
    # Copy the emulation firmware file for x86_64 as it includes NVRAM
    set(FIRMWARE ovmf-x86_64-pure-efi.fd)
    file(
        COPY ${CMAKE_SOURCE_DIR}/third_party/firmware/tianocore/${FIRMWARE}
        DESTINATION ${CMAKE_BINARY_DIR}/emulation/
    )
    set(QEMU_FIRMWARE -drive if=pflash,format=raw,file=${CMAKE_BINARY_DIR}/emulation/${FIRMWARE})
elseif (ARCH STREQUAL "aarch64")
    # The aarch64 emulation firmware needs to be padded to 64m (don't know why, might have to do with using "-machine virt")
    # We also need a separate file for NVRAM.
    set(FIRMWARE ovmf-aarch64.fd)
    execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/emulation/    )
    execute_process(COMMAND truncate -s 64m ${CMAKE_BINARY_DIR}/emulation/${FIRMWARE})
    execute_process(COMMAND truncate -s 64m ${CMAKE_BINARY_DIR}/emulation/nvram.img)
    execute_process(COMMAND dd if=${CMAKE_SOURCE_DIR}/third_party/firmware/tianocore/${FIRMWARE} of=${CMAKE_BINARY_DIR}/emulation/${FIRMWARE} conv=notrunc)
    set(QEMU_FIRMWARE -drive if=pflash,format=raw,file=${CMAKE_BINARY_DIR}/emulation/${FIRMWARE},readonly -drive if=pflash,format=raw,file=$(CMAKE_BINARY_DIR)/emulation/nvram.img)
endif()


###############################################################################
#
# Emulation (QEMU)
#
###############################################################################

set(QEMU_FLAGS -serial mon:stdio -m 8G -net none -drive format=raw,file=${CMAKE_BINARY_DIR}/rainbow-efi-${ARCH}.img)

if (ARCH STREQUAL "x86_64")
    list(APPEND QEMU_FLAGS -accel kvm -cpu Conroe -smp 4)
elseif (ARCH STREQUAL "aarch64")
    list(APPEND QEMU_FLAGS -machine virt -cpu cortex-a53 -device virtio-gpu-pci)
endif()


add_custom_target(run-qemu DEPENDS image)
add_custom_command(
    TARGET run-qemu
	COMMENT "Running Rainbow OS in qemu"
	COMMAND qemu-system-${ARCH} ${QEMU_FLAGS} ${QEMU_FIRMWARE}
)

add_custom_target(run DEPENDS run-qemu)


###############################################################################
#
# Run emulation with GDB
#
###############################################################################

# add_custom_target(debug DEPENDS image)
# add_custom_command(
#     TARGET debug
# 	COMMENT "Running Rainbow OS in qemu (gdb mode)"
#     COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --cyan "To connect the debugger, run gdb -tui -x gdbscript"
#     COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --cyan "To start execution, type continue in gdb"
#     COMMAND qemu-system-${ARCH} ${QEMU_FLAGS} ${QEMU_FIRMWARE} -s -S
#     # COMMAND gdb
#     #     -ex 'file ${CMAKE_BINARY_DIR}/image/efi/rainbow/bootx64.efi'  # TODO: don't hard code
#     #     -ex 'target remote localhost:1234'
#     #     -ex 'break efi_main'
#     #     -ex 'continue'
#         #-ex 'disconnect'
#         #-ex 'quit'
#     )
