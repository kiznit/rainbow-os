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


const auto MEM_1_GB = 0x100000ull;
const auto MEM_4_GB = 0x100000000ull;


static uint64_t pmm_system_memory;      // Detected system memory
static uint64_t pmm_free_memory;        // Free memory
static uint64_t pmm_used_memory;        // Used memory
static uint64_t pmm_unavailable_memory; // Memory that can't be used


// TODO: proper data structure (buddy system or something else)
struct FreeMemory
{
    physaddr_t start;
    physaddr_t end;
};


static FreeMemory s_free_memory[1024];
static int s_free_memory_count;


void pmm_init(const MemoryDescriptor* descriptors, size_t descriptorCount)
{
    for (size_t i = 0; i != descriptorCount; ++i)
    {
        auto entry = &descriptors[i];
        auto start = entry->address;
        auto end = entry->address + entry->size;

        switch (entry->type)
        {
            case MemoryType_Persistent:
            case MemoryType_Unusable:
            case MemoryType_Reserved:
                pmm_unavailable_memory += end - start;
                continue;
            default:
                break;
        }

        pmm_system_memory += entry->size;

//TODO: dont check arch here! Check capabilities...
#if defined(__i386__)
        // In 32 bits mode (non-PAE), we can't address anything above 4 GB
        if (start >= MEM_4_GB)
        {
            pmm_unavailable_memory += end - start;
            continue;
        }

        if (end > MEM_4_GB)
        {
            pmm_unavailable_memory += end - MEM_4_GB;
            end = MEM_4_GB;
        }
#endif

        // Skip first 1 MB (ISA IO SPACE)
        // TODO: that is very x86 specific...
        if (start < MEM_1_GB)
            start = MEM_1_GB;

        // If there is nothing left...
        if (start >= end)
            continue;

        if (entry->type != MemoryType_Available)
        {
            pmm_used_memory += end - start;
            continue;
        }

        s_free_memory[s_free_memory_count].start = start;
        s_free_memory[s_free_memory_count].end = end;
        ++s_free_memory_count;

        pmm_free_memory += end - start;

        if (s_free_memory_count == 1000)
            break;
    }

    // Calculate how much of the system memory we used so far
    pmm_used_memory = pmm_system_memory - pmm_free_memory - pmm_unavailable_memory;

    Log("pmm_init  : check!\n");
    Log("    System Memory: %X (%d MB)\n", pmm_system_memory, pmm_system_memory >> 20);
    Log("    Used Memory  : %X (%d MB)\n", pmm_used_memory, pmm_used_memory >> 20);
    Log("    Free Memory  : %X (%d MB)\n", pmm_free_memory, pmm_free_memory >> 20);
    Log("    Unavailable  : %X (%d MB)\n", pmm_unavailable_memory, pmm_unavailable_memory >> 20);

    if (pmm_free_memory == 0)
    {
        Fatal("No memory available");
    }
}



physaddr_t pmm_allocate_pages(size_t count)
{
    const size_t size = count * MEMORY_PAGE_SIZE;

    for (int i = 0; i != s_free_memory_count; ++i)
    {
        FreeMemory* entry = &s_free_memory[i];
        if (entry->end - entry->start >= size)
        {
            physaddr_t pages = entry->start;
            entry->start += size;
            pmm_free_memory -= size;

            return pages;
        }
    }

    Fatal("Out of physical memory");
}



void pmm_free_pages(physaddr_t address, size_t count)
{
    //TODO
    (void)address;
    (void)count;
}
