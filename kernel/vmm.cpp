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
        m_pageTable->MapPages(frame, m_mmapBegin, 1, PAGE_PRESENT | PAGE_WRITE | PAGE_NX);
    }

    return m_mmapBegin;
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
        m_pageTable->MapPages(frame, m_heapEnd, 1, PAGE_PRESENT | PAGE_WRITE | PAGE_NX);
        m_heapEnd = advance_pointer(m_heapEnd, MEMORY_PAGE_SIZE);
    }

    return result;
}


int VirtualMemoryManager::PageFaultHandler(InterruptContext* context)
{
    // Note: errata: Not-Present Page Faults May Set the RSVD Flag in the Error Code
    // Reference: https://www.intel.com/content/dam/www/public/us/en/documents/specification-updates/xeon-5400-spec-update.pdf
    // The right thing to do is ignore the "RSVD" flag if "P = 0".
    auto error = context->error;

    if (!(error & PAGE_PRESENT))
    {
        const auto address = x86_get_cr2();
        const auto task = g_scheduler->GetCurrentTask();

        // Is this a user stack access?
        if (address >= task->userStackTop && address < task->userStackBottom)
        {
            // We keep the first page as a guard page
            if (address >= task->userStackTop + MEMORY_PAGE_SIZE)
            {
                const auto frame = g_pmm->AllocatePages(1);
                const auto virtualAddress = (void*)align_down(address, MEMORY_PAGE_SIZE);

                task->pageTable.MapPages(frame, virtualAddress, 1, PAGE_PRESENT | PAGE_USER | PAGE_WRITE | PAGE_NX);
                return 1;
            }
            else
            {
                // This is the guard page
                // TODO: raise a "stack overflow" signal / exception
                return 0;
            }
        }
    }

    return 0;
}
