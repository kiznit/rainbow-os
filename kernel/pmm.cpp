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

#include "pmm.hpp"
#include <metal/log.hpp>
#include <rainbow/boot.hpp>


const auto MEM_1_GB = 0x100000ull;
const auto MEM_4_GB = 0x100000000ull;



PhysicalMemoryManager::PhysicalMemoryManager()
:   m_systemBytes(0),
    m_freeBytes(0),
    m_usedBytes(0),
    m_unavailableBytes(0)
{
}



void PhysicalMemoryManager::Initialize(const MemoryDescriptor* descriptors, size_t descriptorCount)
{
    for (size_t i = 0; i != descriptorCount; ++i)
    {
        auto entry = &descriptors[i];
        auto start = entry->address;
        auto end = entry->address + entry->size;

        m_systemBytes += entry->size;

        switch (entry->type)
        {
            case MemoryType_Persistent:
            case MemoryType_Unusable:
            case MemoryType_Reserved:
                m_unavailableBytes += end - start;
                continue;
            default:
                break;
        }

        // If there is nothing left...
        if (start >= end)
            continue;

        if (entry->type != MemoryType_Available)
        {
            m_usedBytes += end - start;
            continue;
        }

        m_freeMemory[m_freeMemoryCount].start = start;
        m_freeMemory[m_freeMemoryCount].end = end;
        ++m_freeMemoryCount;

        m_freeBytes += end - start;

        if (m_freeMemoryCount == 1000)
            break;
    }

    // Calculate how much of the system memory we used so far
    m_usedBytes = m_systemBytes - m_freeBytes - m_unavailableBytes;

    Log("pmm_init  : check!\n");
    Log("    System Memory: %X (%d MB)\n", m_systemBytes, m_systemBytes >> 20);
    Log("    Used Memory  : %X (%d MB)\n", m_usedBytes, m_usedBytes >> 20);
    Log("    Free Memory  : %X (%d MB)\n", m_freeBytes, m_freeBytes >> 20);
    Log("    Unavailable  : %X (%d MB)\n", m_unavailableBytes, m_unavailableBytes >> 20);

    if (m_freeBytes == 0)
    {
        Fatal("No memory available");
    }
}



physaddr_t PhysicalMemoryManager::AllocatePages(size_t count)
{
    const size_t size = count * MEMORY_PAGE_SIZE;

    for (int i = 0; i != m_freeMemoryCount; ++i)
    {
        FreeMemory* entry = &m_freeMemory[i];
        if (entry->end - entry->start >= size)
        {
            physaddr_t pages = entry->start;
            entry->start += size;
            m_freeBytes -= size;

            return pages;
        }
    }

    Fatal("Out of physical memory");
}



void PhysicalMemoryManager::FreePages(physaddr_t address, size_t count)
{
    //TODO
    (void)address;
    (void)count;
}
