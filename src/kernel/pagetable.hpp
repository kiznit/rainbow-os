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

#ifndef _RAINBOW_KERNEL_PAGETABLE_HPP
#define _RAINBOW_KERNEL_PAGETABLE_HPP

#include <memory>
#include <kernel/spinlock.hpp>


// This represents the hardware level page mapping. It is possible that
// some architectures don't actually use page tables in their implementation.
class PageTable
{
public:

    // Clone the current page table (kernel space only)
    std::shared_ptr<PageTable> CloneKernelSpace();

    // Initialize user heap
    void InitUserHeap(void* heapStart, void* heapEnd)
    {
        m_userHeapStart = (char*)heapStart;
        m_userHeapEnd   = (char*)heapEnd;
        m_userHeapBreak = (char*)heapStart;
    }

    // Allocate user pages
    void* AllocateUserPages(int pageCount);

    // Free user pages
    // Returns 0 on success, otherwise an errno code
    int FreeUserPages(void* address, int pageCount);


#if defined(__i386__) || defined(__x86_64__)
    explicit PageTable(uintptr_t cr3)
    :   m_cr3(cr3),
        m_userHeapStart(0),
        m_userHeapEnd(0),
        m_userHeapBreak(0)
    {
    }

    uintptr_t m_cr3;
#endif


private:
    // Do we need a RW lock here?
    Spinlock    m_lock;             // Ensures only one CPU is manipulating the PageTable at once
    char*       m_userHeapStart;    // Start of user heap space
    char*       m_userHeapEnd;      // End of user heap space
    char*       m_userHeapBreak;    // Current user heap break - TODO: need better way than this
};


#endif
