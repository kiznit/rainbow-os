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

#ifndef _RAINBOW_KERNEL_VMM_HPP
#define _RAINBOW_KERNEL_VMM_HPP

#include <metal/arch.hpp>
#include "pagetable.hpp"


class VirtualMemoryManager
{
public:
    void Initialize();

    //TODO: we might need a lock here

    // Allocate pages of memory and map them in kernel space
    // Note: don't assume memory is not zeroed out!
    void* AllocatePages(int pageCount);

    // Extend the heap (aka sbrk)
    void* ExtendHeap(intptr_t increment);

    void*       m_heapBegin;        // Start of heap memory
    void*       m_heapEnd;          // End of heap memory

    // TODO: need proper memory management for the mmap region
    void*       m_mmapBegin;        // Start of memory-map region
    void*       m_mmapEnd;          // End of memory-map region

    PageTable*  m_pageTable;        // Kernel page table
};


#endif
