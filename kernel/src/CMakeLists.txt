# Copyright (c) 2022, Thierry Tremblay
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

add_subdirectory(../../metal ../lib/metal)
add_subdirectory(../../third_party/lai ../lib/lai)

set(DEVICES_SRC
    devices/Device.cpp
    devices/DeviceManager.cpp
    devices/PciDevice.cpp
    devices/pci/Vga.cpp
    devices/pci/VirtioGpu.cpp
)

add_executable(kernel
    ${CMAKE_SYSTEM_PROCESSOR}/arch.cpp
    ${CMAKE_SYSTEM_PROCESSOR}/Cpu.cpp
    ${CMAKE_SYSTEM_PROCESSOR}/interrupt.cpp
    ${CMAKE_SYSTEM_PROCESSOR}/interrupt.S
    ${CMAKE_SYSTEM_PROCESSOR}/PageTable.cpp
    ${CMAKE_SYSTEM_PROCESSOR}/SerialPort.cpp
    ${CMAKE_SYSTEM_PROCESSOR}/start.S
    display.cpp
    kernel.cpp
    memory.cpp
    acpi/acpi.cpp
    acpi/lai.cpp
    pci.cpp
    uefi.cpp
    runtime/crt0.cpp
    runtime/malloc.cpp
    ${DEVICES_SRC}
)

target_include_directories(kernel
    PRIVATE ${PROJECT_SOURCE_DIR}/include
    PRIVATE ${PROJECT_SOURCE_DIR}/src
    PRIVATE ${PROJECT_SOURCE_DIR}/../third_party
)

set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/${CMAKE_SYSTEM_PROCESSOR}/kernel.lds")

target_link_libraries(kernel PRIVATE lai metal)

set_target_properties(kernel PROPERTIES LINK_DEPENDS ${LINKER_SCRIPT})

set_property(TARGET kernel PROPERTY C_STANDARD 17)
set_property(TARGET kernel PROPERTY CXX_STANDARD 20)

target_compile_definitions(kernel PRIVATE ARCH=${CMAKE_SYSTEM_PROCESSOR})
target_compile_options(kernel PRIVATE -fno-strict-aliasing -fwrapv)
target_compile_options(kernel PRIVATE -Wall -Wextra -Werror -Wimplicit-fallthrough)

target_link_options(kernel PRIVATE -Wl,-no-pie -Wl,-T,${LINKER_SCRIPT})
target_link_options(kernel PRIVATE -Wl,--warn-common -Wl,--no-undefined -Wl,--fatal-warnings)
