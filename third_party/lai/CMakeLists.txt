# Copyright (c) 2023, Thierry Tremblay
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
project(lai VERSION 0.0.1)

include(FetchContent)

FetchContent_Declare(
    lai
    GIT_REPOSITORY https://github.com/managarm/lai.git
    GIT_TAG        8cc0b535e7c52dcda8f2afb6a7715b0f3bc76bd1
)

FetchContent_MakeAvailable(lai)

file(GLOB LAI_SRC
    ${lai_SOURCE_DIR}/core/*.c
    ${lai_SOURCE_DIR}/drivers/*.c
    ${lai_SOURCE_DIR}/helpers/*.c
)

add_library(lai STATIC ${LAI_SRC})

target_compile_options(lai PRIVATE -Wno-format)

target_include_directories(lai
    PUBLIC
        ${lai_SOURCE_DIR}/include
)
