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

#include "pmm.hpp"
#include <iterator>
#include <metal/log.hpp>
#include <rainbow/boot.hpp>



// TODO: proper data structure (buddy system or something else)
struct FreeMemory
{
    physaddr_t start;
    physaddr_t end;
};

static FreeMemory s_freeMemory[1024];
static int        s_freeMemoryCount;
static physaddr_t s_systemBytes;      // Detected system memory
static physaddr_t s_freeBytes;        // Free memory
static physaddr_t s_usedBytes;        // Used memory
static physaddr_t s_unavailableBytes; // Memory that can't be used


void pmm_initialize(const MemoryDescriptor* descriptors, size_t descriptorCount)
{
    for (size_t i = 0; i != descriptorCount; ++i)
    {
        auto entry = &descriptors[i];
        auto start = entry->address;
        auto end = entry->address + entry->size;

        s_systemBytes += entry->size;

        switch (entry->type)
        {
            case MemoryType_Persistent:
            case MemoryType_Unusable:
            case MemoryType_Reserved:
                s_unavailableBytes += end - start;
                continue;
            default:
                break;
        }

        // If there is nothing left...
        if (start >= end)
            continue;

        if (entry->type != MemoryType_Available)
        {
            s_usedBytes += end - start;
            continue;
        }

#if defined(__i386__) || defined(__x86_64__)
        // Split memory at 1MB into separate blocks for pmm_allocate_frames_under()
        if (start < MEM_1_MB && end > MEM_1_MB)
        {
            s_freeMemory[s_freeMemoryCount].start = start;
            s_freeMemory[s_freeMemoryCount].end = MEM_1_MB;
            ++s_freeMemoryCount;

            start = MEM_1_MB;
        }

        // Split low memory at 1GB separate blocks for pmm_allocate_frames_under()
        if (start < MEM_1_GB && end > MEM_1_GB)
        {
            s_freeMemory[s_freeMemoryCount].start = start;
            s_freeMemory[s_freeMemoryCount].end = MEM_1_GB;
            ++s_freeMemoryCount;

            start = MEM_1_GB;
        }

        // Split low memory at 4GB separate blocks for pmm_allocate_frames_under()
        if (start < MEM_4_GB && end > MEM_4_GB)
        {
            s_freeMemory[s_freeMemoryCount].start = start;
            s_freeMemory[s_freeMemoryCount].end = MEM_4_GB;
            ++s_freeMemoryCount;

            start = MEM_4_GB;
        }
#endif

        s_freeMemory[s_freeMemoryCount].start = start;
        s_freeMemory[s_freeMemoryCount].end = end;
        ++s_freeMemoryCount;

        s_freeBytes += end - start;

        if (s_freeMemoryCount == std::size(s_freeMemory))
            break;
    }

    // Calculate how much of the system memory we used so far
    s_usedBytes = s_systemBytes - s_freeBytes - s_unavailableBytes;

    Log("pmm_initialize: check!\n");
    Log("    System Memory: %016jX (%jd MB)\n", s_systemBytes, s_systemBytes >> 20);
    Log("    Used Memory  : %016jX (%jd MB)\n", s_usedBytes, s_usedBytes >> 20);
    Log("    Free Memory  : %016jX (%jd MB)\n", s_freeBytes, s_freeBytes >> 20);
    Log("    Unavailable  : %016jX (%jd MB)\n", s_unavailableBytes, s_unavailableBytes >> 20);

    if (s_freeBytes == 0)
    {
        Fatal("No memory available");
    }
}



physaddr_t pmm_allocate_frames(size_t count)
{
    const size_t size = count * MEMORY_PAGE_SIZE;

    for (int i = 0; i != s_freeMemoryCount; ++i)
    {
        FreeMemory* entry = &s_freeMemory[i];

#if defined(__i386__) || defined(__x86_64__)
        // Skip low memory, leave it for pmm_allocate_frames_low()
        if (entry->end <= MEM_1_MB)
        {
            continue;
        }
#endif

        if (entry->end - entry->start >= size)
        {
            physaddr_t frames = entry->start;
            entry->start += size;
            s_freeBytes -= size;

            return frames;
        }
    }

    Fatal("Out of physical memory");
}


#if defined(__i386__) || defined(__x86_64__)

physaddr_t pmm_allocate_frames_under(size_t count, physaddr_t limit)
{
    const size_t size = count * MEMORY_PAGE_SIZE;

    for (int i = 0; i != s_freeMemoryCount; ++i)
    {
        FreeMemory* entry = &s_freeMemory[i];

        // Is this low memory?
        if (entry->end > limit)
        {
            continue;
        }

        if (entry->end - entry->start >= size)
        {
            physaddr_t frames = entry->start;
            entry->start += size;
            s_freeBytes -= size;

            return frames;
        }
    }

    Fatal("Out of physical memory");
}

#endif


void pmm_free_frames(physaddr_t frames, size_t count)
{
    //TODO
    (void)frames;
    (void)count;
}
