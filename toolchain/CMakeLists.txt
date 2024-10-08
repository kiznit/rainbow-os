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

set(LLVM_VERSION 16.0.3)

cmake_minimum_required(VERSION 3.21)
project(RainbowOsToolchain VERSION ${LLVM_VERSION})
include(ExternalProject)


ExternalProject_Add(
    llvm
    PREFIX llvm-${LLVM_VERSION}
    # Download step
    URL https://github.com/llvm/llvm-project/releases/download/llvmorg-${LLVM_VERSION}/llvm-project-${LLVM_VERSION}.src.tar.xz
    URL_HASH SHA1=0b5d889bc0027bf049868e22a537d0a9e67e23b3
    # Update/patch step
    UPDATE_COMMAND ""
    # Configure step
    SOURCE_SUBDIR llvm
    CMAKE_CACHE_ARGS
        -DLLVM_ENABLE_PROJECTS:String=clang\\;lld
        -DLLVM_ENABLE_RUNTIMES:String=compiler-rt
        -DLLVM_TARGETS_TO_BUILD:String=AArch64\\;X86
        -DLLVM_USE_LINKER:String=gold
        -DCMAKE_BUILD_TYPE:String=Release
        -DCMAKE_INSTALL_PREFIX:String=${CMAKE_INSTALL_PREFIX}
)
