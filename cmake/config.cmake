#   Copyright (c) 2025, Thierry Tremblay
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

###############################################################################
#
# Machine
#
################################################################################

set(VALID_MACHINE generic raspi3)

if (NOT DEFINED MACHINE)
    set(MACHINE generic)
    message("MACHINE set to: ${MACHINE}")
else()
    message("MACHINE is: ${MACHINE}")
endif()

if (NOT MACHINE IN_LIST VALID_MACHINE)
    message(FATAL_ERROR "MACHINE must be one of ${VALID_MACHINE}")
endif()

if (MACHINE STREQUAL "raspi3")
    set(ARCH aarch64)
endif()


###############################################################################
#
# Arch
#
################################################################################

set(VALID_ARCH aarch64 x86_64)

if (NOT DEFINED ARCH)
    set(ARCH ${CMAKE_SYSTEM_PROCESSOR})
    message("ARCH set to: ${ARCH}")
else()
    message("ARCH is: ${ARCH}")
endif()

if (NOT ARCH IN_LIST VALID_ARCH)
    message(FATAL_ERROR "ARCH must be one of ${VALID_ARCH}")
endif()


###############################################################################
#
# Build options
#
################################################################################

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "release")
    message("CMAKE_BUILD_TYPE set to ${CMAKE_BUILD_TYPE}")
else()
    message("CMAKE_BUILD_TYPE is ${CMAKE_BUILD_TYPE}")
endif()

###############################################################################
#
# Generator
#
################################################################################

find_program(NINJA NAMES ninja)

if (NINJA)
    set(CMAKE_GENERATOR Ninja)
    message("Using Ninja generator")
else()
    message("Using default generator")
endif()
