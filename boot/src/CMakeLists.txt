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

add_subdirectory(../../metal ../lib/metal)

add_executable(boot
    crt0.cpp
    boot.cpp
    EfiConsole.cpp
    malloc.cpp
)

target_include_directories(boot
    PRIVATE ${PROJECT_SOURCE_DIR}/../kernel/include
    PRIVATE ${PROJECT_SOURCE_DIR}/../third_party
)

target_link_libraries(boot PRIVATE metal)

if (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
    target_compile_options(boot PRIVATE -msoft-float -mno-mmx -mno-sse)
    set_target_properties(boot PROPERTIES OUTPUT_NAME "bootx64.efi")
elseif (CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
    target_compile_options(boot PRIVATE -march=armv8-a+nofp)
    set_target_properties(boot PROPERTIES OUTPUT_NAME "bootaa64.efi")
endif()

set_property(TARGET boot PROPERTY C_STANDARD 17)
set_property(TARGET boot PROPERTY CXX_STANDARD 20)

target_compile_options(boot PRIVATE -fno-strict-aliasing -fwrapv)
target_compile_options(boot PRIVATE -Wall -Wextra -Werror -Wimplicit-fallthrough)
