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

#include <kernel/memorymap.hpp>
#include "pagetable.hpp"


static MemoryMap    s_kernelMemoryMap;
static PageTable    s_kernelPageTable;
static PageTablePae s_kernelPageTablePae;

extern char _heap_start[];


void VirtualMemoryManager::Initialize()
{
    m_kernelMemoryMap = &s_kernelMemoryMap;

    m_kernelMemoryMap->m_heapBegin = &_heap_start;
    m_kernelMemoryMap->m_heapEnd = m_kernelMemoryMap->m_heapBegin;

    if (x86_get_cr4() & X86_CR4_PAE)
    {
        m_kernelMemoryMap->m_mmapBegin = (void*)0xFF7FF000;
        m_kernelMemoryMap->m_mmapEnd = m_kernelMemoryMap->m_mmapBegin;
        m_kernelMemoryMap->m_pageTable = &s_kernelPageTablePae;
    }
    else
    {
        m_kernelMemoryMap->m_mmapBegin = (void*)0xFFC00000;
        m_kernelMemoryMap->m_mmapEnd = m_kernelMemoryMap->m_mmapBegin;
        m_kernelMemoryMap->m_pageTable = &s_kernelPageTable;
    }
}
