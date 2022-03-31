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

if (NOT TARGET unittests)
    add_custom_target(unittests COMMAND ${CMAKE_CTEST_COMMAND})
endif()

set(SRC ../src)

add_executable(boot_tests EXCLUDE_FROM_ALL
    ${SRC}/boot.cpp
    ${SRC}/elf.cpp
    ${SRC}/EfiConsole.cpp
    ${SRC}/EfiFile.cpp
    ${SRC}/MemoryMap.cpp
    ${SRC}/uefi.cpp
    ${SRC}/${CMAKE_SYSTEM_PROCESSOR}/VirtualMemory.cpp
    EfiConsole.test.cpp
    MemoryMap.test.cpp
    uefi.test.cpp
)

add_test(boot_tests boot_tests)
add_dependencies(unittests boot_tests)

target_include_directories(boot_tests
    PRIVATE ${SRC}
    PRIVATE ${SRC}/${CMAKE_SYSTEM_PROCESSOR}
    PRIVATE ${PROJECT_SOURCE_DIR}/../kernel/include
)

target_link_libraries(boot_tests PRIVATE metal unittest)

set_property(TARGET boot_tests PROPERTY C_STANDARD 17)
set_property(TARGET boot_tests PROPERTY CXX_STANDARD 20)

target_compile_options(boot_tests PRIVATE -fno-strict-aliasing -fwrapv)
target_compile_options(boot_tests PRIVATE -Wall -Wextra -Werror -Wimplicit-fallthrough)
target_compile_options(boot_tests PRIVATE -Wno-missing-field-initializers)

if (CODE_COVERAGE)
    target_add_converage(boot_tests)
endif()
