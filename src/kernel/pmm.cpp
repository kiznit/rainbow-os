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
#include <array>
#include <kernel/config.hpp>
#include <kernel/vmm.hpp>
#include <metal/helpers.hpp>
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

// Min/max known addresses - TODO: we can probably do better for pmm_map_all_physical_memory()
static physaddr_t s_minAddress = (physaddr_t)-1;
static physaddr_t s_maxAddress = 0;


void pmm_initialize(const MemoryDescriptor* descriptors, size_t descriptorCount)
{
    for (size_t i = 0; i != descriptorCount; ++i)
    {
        auto entry = &descriptors[i];
        auto start = entry->address;
        auto end = entry->address + entry->size;

        s_minAddress = std::min(s_minAddress, start);
        s_maxAddress = std::max(s_maxAddress, end);

        s_systemBytes += entry->size;

        switch (entry->type)
        {
            case MemoryType::Unusable:
                s_unavailableBytes += end - start;
                continue;

            case MemoryType::Persistent:
            case MemoryType::Reserved:
                continue;

            default:
                break;
        }

        // If there is nothing left...
        if (start >= end)
            continue;

        if (entry->type != MemoryType::Available)
        {
            s_usedBytes += end - start;
            continue;
        }

        // Record free bytes before we start splitting the region on memory boundaries
        s_freeBytes += end - start;


#if defined(__i386__) || defined(__x86_64__)
        // Split memory at 1MB into separate blocks for pmm_allocate_frames_under()
        if (start < MEM_1_MB && end > MEM_1_MB)
        {
            s_freeMemory[s_freeMemoryCount].start = start;
            s_freeMemory[s_freeMemoryCount].end = MEM_1_MB;
            ++s_freeMemoryCount;

            start = MEM_1_MB;
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

        if (s_freeMemoryCount == std::ssize(s_freeMemory))
            break;
    }

    // Calculate how much of the system memory we used so far
    s_usedBytes = s_systemBytes - s_freeBytes - s_unavailableBytes;

    // Round min/max address
    s_minAddress = align_down(s_minAddress, MEMORY_PAGE_SIZE);
    s_maxAddress = align_down(s_maxAddress, MEMORY_PAGE_SIZE);

    Log("pmm_initialize: check!\n");
    Log("    System Memory: %016jX (%jd MB)\n", s_systemBytes, s_systemBytes >> 20);
    Log("    Used Memory  : %016jX (%jd MB)\n", s_usedBytes, s_usedBytes >> 20);
    Log("    Free Memory  : %016jX (%jd MB)\n", s_freeBytes, s_freeBytes >> 20);
    Log("    Unavailable  : %016jX (%jd MB)\n", s_unavailableBytes, s_unavailableBytes >> 20);
    Log("    Min Address  : %016jx\n", s_minAddress);
    Log("    Max Address  : %016jx\n", s_maxAddress);

    if (s_freeBytes == 0)
    {
        Fatal("No memory available");
    }

#if defined(__x86_64__)
    pmm_map_all_physical_memory();
#endif
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

    // TODO: need to return failure and let the caller handle it
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

    // TODO: need to return failure and let the caller handle it
    Fatal("Out of physical memory");
}

#endif


void pmm_free_frames(physaddr_t frames, size_t count)
{
    //TODO
    (void)frames;
    (void)count;
}


#if defined(__x86_64__)
void pmm_map_all_physical_memory()
{
    // TODO: we need to look at MTRRs to figure out which ranges
    // can be mapped using large pages or not... right now we might
    // be creating large pages that cover some uncacheable regions (hardware)
    // and that could be a problem...
    //
    // TODO: the framebuffer (and all video memory really) should be mapped
    // as write-combining. Right now we do that outside this direct mapping,
    // but we should do it here as well and use the direct mapping for accessing
    // video memory instead of the "temporary" boot mapping.
    const auto pageCount = (s_maxAddress - s_minAddress) >> MEMORY_PAGE_SHIFT;
    const auto address = advance_pointer(VMA_PHYSICAL_MAP_START, s_minAddress);
    vmm_map_pages(s_minAddress, address, pageCount, PageType::KernelData_RW);
}
#endif
