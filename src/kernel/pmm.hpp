/*
    Copyright (c) 2020, Thierry Tremblay
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

#ifndef _RAINBOW_KERNEL_PMM_HPP
#define _RAINBOW_KERNEL_PMM_HPP

#include <cstddef>
#include <metal/arch.hpp>


class MemoryDescriptor;


const auto MEM_1_MB = 0x000100000ull;
const auto MEM_1_GB = 0x040000000ull;
const auto MEM_4_GB = 0x100000000ull;


// Initialize the physical memory manager
void pmm_initialize(const MemoryDescriptor* descriptors, size_t descriptorCount);

// Allocate physical memory
physaddr_t pmm_allocate_frames(size_t count);


#if defined(__i386__) || defined(__x86_64__)

// Allocate physical memory under the specified address
physaddr_t pmm_allocate_frames_under(size_t count, physaddr_t limit);

#endif



// Free physical memory
void pmm_free_frames(physaddr_t frames, size_t count);


#if defined(__x86_64__)
// Map all physical memory so that it can easily be accessed by the kernel
void pmm_map_all_physical_memory();
#endif

#endif
