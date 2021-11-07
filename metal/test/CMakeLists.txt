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

if (NOT TARGET check)
    add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND})
endif()

set(SRC ../src)

add_executable(metal_tests EXCLUDE_FROM_ALL
    ${SRC}/unicode.cpp
    ${SRC}/log/core.cpp
    ${SRC}/log/stream.cpp
    expected.test.cpp
    LogStream.test.cpp
    unicode.test.cpp
)

add_test(metal_tests metal_tests)
add_dependencies(check metal_tests)

target_include_directories(metal_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(metal_tests PRIVATE unittest)

set_property(TARGET metal_tests PROPERTY C_STANDARD 17)
set_property(TARGET metal_tests PROPERTY CXX_STANDARD 20)

target_compile_options(metal_tests PRIVATE -fno-strict-aliasing -fwrapv)
target_compile_options(metal_tests PRIVATE -Wall -Wextra -Werror -Wimplicit-fallthrough)

if (CODE_COVERAGE)
    target_add_converage(metal_tests)
endif()
