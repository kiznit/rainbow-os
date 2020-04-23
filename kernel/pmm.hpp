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

#include <stddef.h>
#include <metal/arch.hpp>


class MemoryDescriptor;


class PhysicalMemoryManager
{
public:
    PhysicalMemoryManager();

    void Initialize(const MemoryDescriptor* descriptors, size_t descriptorCount);

    // Allocate physical memory
    physaddr_t AllocatePages(size_t count);

    // Free physical memory
    void FreePages(physaddr_t address, size_t count);


private:

    // TODO: proper data structure (buddy system or something else)
    struct FreeMemory
    {
        physaddr_t start;
        physaddr_t end;
    };

    FreeMemory  m_freeMemory[1024];
    int         m_freeMemoryCount;
    uint64_t    m_systemBytes;      // Detected system memory
    uint64_t    m_freeBytes;        // Free memory
    uint64_t    m_usedBytes;        // Used memory
    uint64_t    m_unavailableBytes; // Memory that can't be used
};


#endif
