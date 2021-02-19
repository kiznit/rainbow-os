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

#include "pagetable.hpp"
#include <cerrno>
#include <cstring>
#include <mutex>
#include <kernel/pmm.hpp>
#include <kernel/vmm.hpp>


void* PageTable::AllocateUserPages(int pageCount)
{
    char* virtualAddress;

    {
        std::lock_guard lock(m_lock);

        virtualAddress = m_userHeapBreak;

        // Is there enough space?
        if (virtualAddress + pageCount * MEMORY_PAGE_SIZE > m_userHeapEnd)
        {
            return nullptr;
        }

        // TODO: allocating continuous frames might fail, need better API
        // TODO: error handling
        auto physicalAddress = pmm_allocate_frames(pageCount);

        // Map memory in user space
        // TODO: error handling
        vmm_map_pages(physicalAddress, virtualAddress, pageCount, PAGE_PRESENT | PAGE_USER | PAGE_WRITE | PAGE_NX);

        // Update break
        m_userHeapBreak += pageCount * MEMORY_PAGE_SIZE;
    }

    // Zero memory before returning it to the user
    memset(virtualAddress, 0, pageCount * MEMORY_PAGE_SIZE);

    return virtualAddress;
}


int PageTable::FreeUserPages(void* memory, int pageCount)
{
    std::lock_guard lock(m_lock);

    auto virtualAddress = (char*)memory;

    // Validate range is in the heap region
    if (virtualAddress < m_userHeapStart || virtualAddress + pageCount * MEMORY_PAGE_SIZE > m_userHeapBreak)
    {
        return ENOMEM;
    }

    // TODO: error handling
    vmm_free_pages(virtualAddress, pageCount);

    // TODO: free the VMA range

    return 0;
}
