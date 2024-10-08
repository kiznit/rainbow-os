# Copyright (c) 2024, Thierry Tremblay
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

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

include(config)

if (CODE_COVERAGE)
include(code-coverage)
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

if (MACHINE STREQUAL "raspi3")
    set(TOOLCHAIN_MACHINE "rpi3")
else()
    set(TOOLCHAIN_MACHINE "none")
endif()

ExternalProject_Add(
    external_boot
    BUILD_ALWAYS 1
    PREFIX ${CMAKE_BINARY_DIR}/boot
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/boot
    BINARY_DIR ${CMAKE_BINARY_DIR}/boot
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_TOOLCHAIN_FILE=${CMAKE_SOURCE_DIR}/cmake/toolchains/${ARCH}-${TOOLCHAIN_MACHINE}-uefi.cmake -DMACHINE=${MACHINE} -B ${CMAKE_BINARY_DIR}/boot
    BUILD_COMMAND ${GENERATOR_COMMAND}
    INSTALL_COMMAND ""
)

ExternalProject_Add(
    external_kernel
    BUILD_ALWAYS 1
    PREFIX ${CMAKE_BINARY_DIR}/kernel
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/kernel
    BINARY_DIR ${CMAKE_BINARY_DIR}/kernel
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_TOOLCHAIN_FILE=${CMAKE_SOURCE_DIR}/cmake/toolchains/${ARCH}-${TOOLCHAIN_MACHINE}-elf.cmake -DMACHINE=${MACHINE} -B ${CMAKE_BINARY_DIR}/kernel
    BUILD_COMMAND ${GENERATOR_COMMAND}
    INSTALL_COMMAND ""
)


###############################################################################
#
# Unit tests
#
###############################################################################

enable_testing()

add_custom_target(unittests)
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} DEPENDS unittests)

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
# Raspberry Pi
#
###############################################################################

if (MACHINE STREQUAL "raspi3")
    add_subdirectory(third_party/firmware/rpi3 external/rpi3)
    ExternalProject_Get_Property(rpi3_firmware SOURCE_DIR)

    add_custom_target(image_rpi3 DEPENDS rpi3_firmware)
    add_custom_command(
        TARGET image_rpi3
        COMMENT "Copying Raspberry Pi 3 UEFI firmware"
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/image
        COMMAND ${CMAKE_COMMAND} -E copy ${SOURCE_DIR}/* ${CMAKE_BINARY_DIR}/image
    )

    set(IMAGE_EXTRA_DEPS ${IMAGE_EXTRA_DEPS} image_rpi3)
endif()


###############################################################################
#
# UEFI disk image
#
###############################################################################

add_custom_target(image_uefi DEPENDS external_boot external_kernel ${IMAGE_EXTRA_DEPS})
add_custom_command(
    TARGET image_uefi
    COMMENT "Copying bootloader and kernel"
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/image/efi/boot
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/boot/src/*.efi ${CMAKE_BINARY_DIR}/image/efi/boot/
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/image/efi/rainbow
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/boot/src/*.efi ${CMAKE_BINARY_DIR}/image/efi/rainbow/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/kernel/src/kernel ${CMAKE_BINARY_DIR}/image/efi/rainbow/
)

add_custom_target(image DEPENDS image_uefi)
add_custom_command(
    TARGET image
    POST_BUILD
    COMMENT "Building UEFI disk image"
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

set(QEMU_FLAGS -serial mon:stdio -display gtk,zoom-to-fit=off)
set(QEMU_IMAGE -drive format=raw,file=${CMAKE_BINARY_DIR}/rainbow-efi-${ARCH}.img)

if (ARCH STREQUAL "x86_64")
    set(QEMU_MACHINE -machine type=q35 -m 8G -cpu Conroe -smp 4) # TODO: remove VGA once virtio-gpu-pci works: -vga none
    set(QEMU_DEVICES -device virtio-gpu-pci)
    list(APPEND QEMU_FLAGS -accel kvm -boot order=d)
elseif (ARCH STREQUAL "aarch64")
    set(QEMU_MACHINE -machine virt -m 8G -cpu cortex-a53 -smp 4)
    set(QEMU_DEVICES -device virtio-gpu-pci)
endif()

add_custom_target(run-qemu DEPENDS image)
add_custom_command(
    TARGET run-qemu
	COMMENT "Running Rainbow OS in qemu"
	COMMAND qemu-system-${ARCH} ${QEMU_FLAGS} ${QEMU_MACHINE} ${QEMU_DEVICES} ${QEMU_FIRMWARE} ${QEMU_IMAGE} #-d int,cpu_reset -monitor telnet:127.0.0.1:55555,server,nowait
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
