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

#include "vmm.hpp"
#include <cassert>
#include <cstring>
#include <metal/helpers.hpp>
#include <metal/log.hpp>
#include "config.hpp"
#include "pagetable.hpp"
#include "pmm.hpp"

// TODO: we aren't even using these heap definitions... do we want to keep them?
static void*      s_heapBegin;        // Start of heap memory
static void*      s_heapEnd;          // End of heap memory
// TODO: need proper memory management for the mmap region
static void*      s_mmapBegin;        // Start of memory-map region
static void*      s_mmapEnd;          // End of memory-map region
static PageTable  s_pageTable;        // Kernel page table


void vmm_initialize()
{
    s_heapBegin = s_heapEnd = VMA_HEAP_START;
    s_mmapBegin = s_mmapEnd = VMA_HEAP_END;

    s_pageTable.cr3 = x86_get_cr3();

    Log("vmm_initialize: check!\n");
}


// TODO: make sure we don't start stepping over the heap!
void* vmm_allocate_pages(int pageCount)
{
    // TODO: need critical section here...

    // TODO: provide an API to allocate 'x' continuous frames
    for (auto i = 0; i != pageCount; ++i)
    {
        auto frame = pmm_allocate_frames(1);
        s_mmapBegin = advance_pointer(s_mmapBegin, -MEMORY_PAGE_SIZE);

        // TODO: verify return value
        s_pageTable.MapPages(frame, s_mmapBegin, 1, PAGE_PRESENT | PAGE_WRITE | PAGE_NX);

        // TODO: we should keep a pool of zero-ed memory
        memset(s_mmapBegin, 0, MEMORY_PAGE_SIZE);
    }

    return s_mmapBegin;
}


void* vmm_map_pages(physaddr_t address, int pageCount, uint64_t flags)
{
    // TODO: need critical section here...
    assert(!(address & (MEMORY_PAGE_SIZE-1)));

    void* vma = advance_pointer(s_mmapBegin, -(pageCount * MEMORY_PAGE_SIZE));
    auto frame = address;

    // TODO: verify return value
    s_pageTable.MapPages(frame, vma, pageCount, PAGE_PRESENT | PAGE_WRITE | PAGE_NX | flags);

    s_mmapBegin = vma;

    return vma;
}


void vmm_free_pages(void* address, int pageCount)
{
    // TODO: need critical section here...
    // TODO: TLB shutdown (SMP)
    (void)address;
    (void)pageCount;
}
