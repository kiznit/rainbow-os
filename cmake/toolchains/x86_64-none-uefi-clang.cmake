#   Copyright (c) 2023, Thierry Tremblay
#   All rights reserved.
#
#   Redistribution and use in source and binary forms, with or without
#   modification, are permitted provided that the following conditions are met:
#
#   * Redistributions of source code must retain the above copyright notice, this
#     list of conditions and the following disclaimer.
#
#   * Redistributions in binary form must reproduce the above copyright notice,
#     this list of conditions and the following disclaimer in the documentation
#     and/or other materials provided with the distribution.
#
#   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
#   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
#   FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
#   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
#   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
#   CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
#   OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

find_program(CLANG NAMES clang)

if (CLANG)
    message("Found clang: ${CLANG}")
    set(CMAKE_ASM_COMPILER ${CLANG})
    set(CMAKE_C_COMPILER ${CLANG})
    set(CMAKE_CXX_COMPILER ${CLANG})
else()
    message(FATAL_ERROR "Could not detect compiler to use")
endif()

include(${CMAKE_CURRENT_LIST_DIR}/x86_64-none-uefi-common.cmake)

set(CMAKE_ASM_FLAGS "-target x86_64-unknown-windows ${CMAKE_ASM_FLAGS}")
set(CMAKE_C_FLAGS "-target x86_64-unknown-windows ${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "-target x86_64-unknown-windows ${CMAKE_CXX_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "-nostdlib -fuse-ld=lld -Wl,-entry:_start -Wl,-subsystem:efi_application")
