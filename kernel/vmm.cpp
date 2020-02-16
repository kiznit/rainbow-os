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
#include <kernel/kernel.hpp>



// TODO: make sure we don't start stepping over the heap!
void* VirtualMemoryManager::AllocatePages(int pageCount)
{
    // TODO: provide an API to allocate 'x' continuous frames
    for (auto i = 0; i != pageCount; ++i)
    {
        auto frame = g_pmm->AllocatePages(1);
        m_mmapBegin = advance_pointer(m_mmapBegin, -MEMORY_PAGE_SIZE);
        m_pageTable->MapPage(frame, m_mmapBegin);
    }

    auto memory = m_mmapBegin;

    memset(memory, 0, pageCount * MEMORY_PAGE_SIZE);

    return memory;
}


// TODO: make sure we don't extend further than allowed (reaching memory map region or something!)
void* VirtualMemoryManager::ExtendHeap(intptr_t increment)
{
    //TODO: support negative values?
    assert(increment >= 0);

    const size_t pageCount = align_up(increment, MEMORY_PAGE_SIZE) >> MEMORY_PAGE_SHIFT;

    auto result = m_heapEnd;

    // TODO: provide an API to allocate 'x' pages and map them continuously in virtual space
    for (size_t i = 0; i != pageCount; ++i)
    {
        auto frame = g_pmm->AllocatePages(1);
        m_pageTable->MapPage(frame, m_heapEnd);
        m_heapEnd = advance_pointer(m_heapEnd, MEMORY_PAGE_SIZE);
    }

    return result;
}
