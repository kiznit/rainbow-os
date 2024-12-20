# Copyright (c) 2025, Thierry Tremblay
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the followi``ng disclaimer in the documentation
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

cmake_minimum_required(VERSION 3.10)
project(RainbowOS VERSION 0.0.1)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
include(config)

###############################################################################
#
# Bootloader and kernel require a specific toolchain and cannot be built
# using the same compiler used for other Rainbow-OS components. Unfortunately
# cmake cannot handle multiple toolchains from within the same project. We use
# the usual workaround of building external projects to handle this situation.
#
################################################################################

include(ExternalProject)

find_program(CLANG NAMES clang)
find_program(MINGW NAMES x86_64-w64-mingw32-gcc)

if (CLANG)
    message("Using clang for bootloader")
    set(bootToolchainTuple ${ARCH}-unknown-uefi-clang)
elseif (ARCH STREQUAL "x86_64" AND MINGW)
    message("Using mingw for bootloader")
set(bootToolchainTuple ${ARCH}-unknown-uefi-mingw)
else()
    message(FATAL_ERROR "No suitable toolchain found for bootloader")
endif()

# Stolen from https://www.vinnie.work/blog/2020-11-17-cmake-eval
function(add_multi_target_component srcRelPrefix project toolchainTuple)
    set(projectToolchainTuple ${project}-${toolchainTuple})
    set(toolchainFile ${CMAKE_SOURCE_DIR}/cmake/toolchains/${toolchainTuple}.cmake)
    set(outputDir ${CMAKE_BINARY_DIR}/${toolchainTuple}/${project})
    ExternalProject_Add(
        ${project}
        BUILD_ALWAYS 1
        PREFIX ${outputDir}
        SOURCE_DIR ${CMAKE_SOURCE_DIR}/${srcRelPrefix}/${project}
        BINARY_DIR ${outputDir}
        CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_TOOLCHAIN_FILE=${toolchainFile} -B ${outputDir}
        INSTALL_COMMAND ""
    )
endfunction()


add_multi_target_component(. boot ${bootToolchainTuple})

###############################################################################
#
# UEFI disk image
#
###############################################################################

add_custom_target(image_uefi DEPENDS boot ${IMAGE_EXTRA_DEPS})
add_custom_command(
    TARGET image_uefi
    COMMENT "Copying bootloader and kernel"
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/image/efi/boot
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/${bootToolchainTuple}/boot/src/*.efi ${CMAKE_BINARY_DIR}/image/efi/boot/
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/image/efi/rainbow
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/${bootToolchainTuple}/boot/src/*.efi ${CMAKE_BINARY_DIR}/image/efi/rainbow/
#    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/kernel/src/kernel ${CMAKE_BINARY_DIR}/image/efi/rainbow/
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
    set(QEMU_FIRMWARE -drive if=pflash,format=raw,file=${CMAKE_BINARY_DIR}/emulation/${FIRMWARE},readonly=on -drive if=pflash,format=raw,file=$(CMAKE_BINARY_DIR)/emulation/nvram.img)
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

###############################################################################
#
# Default run target
#
###############################################################################

add_custom_target(run DEPENDS run-qemu)
