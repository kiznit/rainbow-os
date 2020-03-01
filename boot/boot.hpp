/*
    Copyright (c) 2018, Thierry Tremblay
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _RAINBOW_BOOT_BOOT_HPP
#define _RAINBOW_BOOT_BOOT_HPP

#include <stddef.h>
#include <metal/arch.hpp>
#include <metal/crt.hpp>
#include <metal/helpers.hpp>
#include <metal/log.hpp>
#include <rainbow/boot.hpp>
#include "memory.hpp"

class IConsole;


// Globals
extern BootInfo g_bootInfo;
extern IConsole* g_console;


// Allocate memory pages of size MEMORY_PAGE_SIZE.
// 'maxAddress' is exclusive (all memory will be below that address)
// Returns NULL on failure / out of memory (so make sure the implementation doesn't return 0 as valid memory).
void* AllocatePages(size_t pageCount, physaddr_t maxAddress = MAX_ALLOC_ADDRESS);


// Boot
void Boot(void* kernel, size_t kernelSize);


#endif
