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

enable_language(ASM)

if (NOT DEFINED ARCH)
    set(ARCH ${CMAKE_SYSTEM_PROCESSOR})
endif()

add_subdirectory(../../metal ../external/metal)
add_subdirectory(../../third_party/lai ../external/lai)

set(DEVICES_SRC
    devices/Device.cpp
    devices/DeviceManager.cpp
    devices/PciDevice.cpp
    devices/pci/Vga.cpp
    devices/pci/VirtioGpu.cpp
)

set(KERNEL_SRC
    ${ARCH}/arch.cpp
    ${ARCH}/Cpu.cpp
    ${ARCH}/CpuContext.S
    ${ARCH}/exception.cpp
    ${ARCH}/Interrupt.cpp
    ${ARCH}/interrupt.S
    ${ARCH}/PageTable.cpp
    ${ARCH}/SerialPort.cpp
    ${ARCH}/start.S
    ${ARCH}/task.cpp
    display.cpp
    kernel.cpp
    memory.cpp
    acpi/Acpi.cpp
    acpi/lai.cpp
    pci.cpp
    Scheduler.cpp
    Spinlock.cpp
    Task.cpp
    uefi.cpp
    runtime/crt0.cpp
    runtime/malloc.cpp
)

if (ARCH STREQUAL "x86_64")
    list(APPEND KERNEL_SRC
    ${ARCH}/InterruptTable.cpp
    ${ARCH}/devices/Apic.cpp
    ${ARCH}/devices/Hpet.cpp
    ${ARCH}/devices/IoApic.cpp
    ${ARCH}/devices/Pic.cpp
    ${ARCH}/devices/Pit.cpp
)
endif()

if (ARCH STREQUAL "aarch64")
    list(APPEND KERNEL_SRC
        ${ARCH}/devices/GenericTimer.cpp
        ${ARCH}/devices/GicCpuInterface.cpp
        ${ARCH}/devices/GicDistributor.cpp
    )
endif()

if (MACHINE STREQUAL "raspi3")
    list(APPEND KERNEL_SRC ${ARCH}/raspi3.cpp)
endif()

add_executable(kernel
    ${KERNEL_SRC}
    ${DEVICES_SRC}
)

target_include_directories(kernel
    PRIVATE ${PROJECT_SOURCE_DIR}/include
    PRIVATE ${PROJECT_SOURCE_DIR}/src
    PRIVATE ${PROJECT_SOURCE_DIR}/src/${ARCH}
    PRIVATE ${PROJECT_SOURCE_DIR}/../third_party
)

set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/${ARCH}/kernel.lds")

target_link_libraries(kernel PRIVATE lai metal)

set_target_properties(kernel PROPERTIES LINK_DEPENDS ${LINKER_SCRIPT})

set_property(TARGET kernel PROPERTY C_STANDARD 17)
set_property(TARGET kernel PROPERTY CXX_STANDARD 20)

string(TOUPPER CONFIG_${MACHINE} CONFIG_MACHINE)
target_compile_definitions(kernel PRIVATE ARCH=${ARCH} ${CONFIG_MACHINE}=1)
target_compile_options(kernel PRIVATE -fno-strict-aliasing -fwrapv)
target_compile_options(kernel PRIVATE -Wall -Wextra -Werror -Wimplicit-fallthrough)

target_link_options(kernel PRIVATE -Wl,-no-pie -Wl,-T,${LINKER_SCRIPT})
target_link_options(kernel PRIVATE -Wl,--warn-common -Wl,--no-undefined -Wl,--fatal-warnings)
