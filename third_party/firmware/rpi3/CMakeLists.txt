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

set(RPI3_VERSION 1.37)

cmake_minimum_required(VERSION 3.21)
project(Raspi3Firmware VERSION ${RPI3_VERSION})
include(ExternalProject)

ExternalProject_Add(
    rpi3_firmware
    PREFIX rpi3-${RPI3_VERSION}
    # Download step
    URL https://github.com/pftf/RPi3/releases/download/v1.37/RPi3_UEFI_Firmware_v1.37.zip
    URL_HASH SHA1=f4e9ad82878931aa8ee187b0c8cc92e26f95054e
    # Update/patch step
    UPDATE_COMMAND ""
    # Configure step
    CONFIGURE_COMMAND ""
    # Build step
    BUILD_COMMAND ""
    # Install step
    INSTALL_COMMAND ""
)
