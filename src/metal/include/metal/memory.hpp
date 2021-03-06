/*
    Copyright (c) 2021, Thierry Tremblay
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

#ifndef _RAINBOW_METAL_MEMORY_HPP
#define _RAINBOW_METAL_MEMORY_HPP


// We use abstract memory types because each architecture does it very differently.
// constexprs are used to efficiently generate the required arch-specific page flags.

enum PageType
{
    KernelCode,
    KernelData_RO,
    KernelData_RW,
    UserCode,
    UserData_RO,
    UserData_RW,
    MMIO,               // Memory Mapped Device I/O (uncacheable memory)
    VideoFramebuffer,   // Video Memory (write-combining)
};


#if defined(__i386__) || defined(__x86_64__)
#include "x86/memory.hpp"
#elif defined(__arm__)
#include "arm/memory.hpp"
#elif defined(__aarch64__)
#include "aarch64/memory.hpp"
#else
#error Unknown arch
#endif


#endif
