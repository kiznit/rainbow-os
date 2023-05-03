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

include(FetchContent)

# TODO: it would be nice to check if expected is alredy installed?
FetchContent_Declare(
    expected
    GIT_REPOSITORY https://github.com/kiznit/expected.git
    GIT_TAG        v0.12.3
)

set(expected_BUILD_TESTS OFF)

FetchContent_MakeAvailable(expected)

if (NOT DEFINED ARCH)
    set(ARCH ${CMAKE_SYSTEM_PROCESSOR})
endif()

set(METAL_SRC_CORE
    unicode.cpp
    arch/${ARCH}/cpu.cpp
    graphics/Edid.cpp
	graphics/GraphicsConsole.cpp
	graphics/PixelFormat.cpp
	graphics/SimpleDisplay.cpp
    graphics/Surface.cpp
	graphics/VgaFont.cpp
    log/core.cpp
    log/stream.cpp
)

set(METAL_SRC_STDLIB
    c/abort.c
    c/assert.cpp
    c/memcmp.c
    c/memcpy.c
    c/memset.c
    c/strcmp.c
    c/strlen.c
    c/wcslen.c
    c++/runtime.cpp
)

set(METAL_SRC ${METAL_SRC_CORE})
if (CMAKE_CROSSCOMPILING)
    list(APPEND METAL_SRC ${METAL_SRC_STDLIB})
endif()

add_library(metal STATIC ${METAL_SRC})

target_include_directories(metal
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(metal
    PUBLIC
        expected
)

if (CMAKE_CROSSCOMPILING)
    target_include_directories(metal
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/../include/c
            ${CMAKE_CURRENT_SOURCE_DIR}/../include/c++
    )
endif()


set_property(TARGET metal PROPERTY C_STANDARD 17)
set_property(TARGET metal PROPERTY CXX_STANDARD 20)

target_compile_options(metal PRIVATE -fno-strict-aliasing -fwrapv)
