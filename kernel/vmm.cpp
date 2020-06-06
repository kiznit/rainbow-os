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
#include <metal/crt.hpp>
#include <metal/helpers.hpp>
#include <metal/log.hpp>
#include "config.hpp"
#include "pagetable.hpp"
#include "pmm.hpp"

static void*      m_heapBegin;        // Start of heap memory
static void*      m_heapEnd;          // End of heap memory
// TODO: need proper memory management for the mmap region
static void*      m_mmapBegin;        // Start of memory-map region
static void*      m_mmapEnd;          // End of memory-map region
static PageTable  m_pageTable;        // Kernel page table


void vmm_initialize()
{
    m_heapBegin = m_heapEnd = VMA_HEAP_START;
    m_mmapBegin = m_mmapEnd = VMA_HEAP_END;

    m_pageTable.cr3 = x86_get_cr3();

    Log("vmm_initialize: check!\n");
}


// TODO: make sure we don't start stepping over the heap!
void* vmm_allocate_pages(int pageCount)
{
    // TODO: provide an API to allocate 'x' continuous frames
    for (auto i = 0; i != pageCount; ++i)
    {
        auto frame = pmm_allocate_frames(1);
        m_mmapBegin = advance_pointer(m_mmapBegin, -MEMORY_PAGE_SIZE);
        m_pageTable.MapPages(frame, m_mmapBegin, 1, PAGE_PRESENT | PAGE_WRITE | PAGE_NX);
    }

    return m_mmapBegin;
}



void vmm_free_pages(void* address, int pageCount)
{
    // TODO
    (void)address;
    (void)pageCount;
}



// TODO: make sure we don't extend further than allowed (reaching memory map region or something!)
void* vmm_extend_heap(intptr_t increment)
{
    //TODO: support negative values?
    assert(increment >= 0);

    const size_t pageCount = align_up(increment, MEMORY_PAGE_SIZE) >> MEMORY_PAGE_SHIFT;

    auto result = m_heapEnd;

    // TODO: provide an API to allocate 'x' pages and map them continuously in virtual space
    for (size_t i = 0; i != pageCount; ++i)
    {
        auto frame = pmm_allocate_frames(1);
        m_pageTable.MapPages(frame, m_heapEnd, 1, PAGE_PRESENT | PAGE_WRITE | PAGE_NX);
        m_heapEnd = advance_pointer(m_heapEnd, MEMORY_PAGE_SIZE);
    }

    return result;
}
