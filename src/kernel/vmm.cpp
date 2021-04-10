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
#include <cassert>
#include <cstring>
#include <kernel/config.hpp>
#include <metal/helpers.hpp>
#include <metal/log.hpp>
#include "config.hpp"
#include "pagetable.hpp"
#include "pmm.hpp"

#if defined(__i386__) || defined(__x86_64__)
// TODO: arch specific does not belong here
#include <metal/x86/cpu.hpp>
#endif



// TODO: are we happy with this global?
std::shared_ptr<PageTable> g_kernelPageTable;


// TODO: make sure the heap and mmap allocations don't cross each others
//static char* s_heapBegin = (char*)VMA_HEAP_START;  // Start of heap memory
static char* s_heapEnd   = (char*)VMA_HEAP_START;  // End of heap memory
static char* s_heapBreak = (char*)VMA_HEAP_START;  // Current break
// TODO: need proper memory management for the mmap region
static char* s_mmapBegin = (char*)VMA_HEAP_END;    // Start of memory-map region
//static char* s_mmapEnd   = (char*)VMA_HEAP_END;    // End of memory-map region


void vmm_initialize()
{
     // This is the first heap allocation made by the kernel...
     // The heap must be initialized and ready to go before we get to this point.

#if defined(__i386__) || defined(__x86_64__)
// TODO: arch specific does not belong here
     g_kernelPageTable = std::make_shared<PageTable>(x86_get_cr3());
#endif

     Log("vmm_initialize: check!\n");
}


void* vmm_sbrk(ptrdiff_t size)
{
    // TODO: need critical section here...

    if (s_heapBreak + size <= s_mmapBegin)
    {
        auto p = (void*)s_heapBreak;

        if (size > 0)
        {
            // We must make sure we have enough allocated pages to increase the break
            const int pageCount = align_up(s_heapBreak + size - s_heapEnd, MEMORY_PAGE_SIZE) >> MEMORY_PAGE_SHIFT;

            if (pageCount > 0)
            {
                auto physicalAddress = pmm_allocate_frames(pageCount);
                // TODO: error handling
                vmm_map_pages(physicalAddress, s_heapEnd, pageCount, PageType::KernelData_RW);

                // TODO: we should keep a pool of zero-ed memory
                memset(s_heapEnd, 0, pageCount * MEMORY_PAGE_SIZE);

                s_heapEnd += pageCount * MEMORY_PAGE_SIZE;
            }
        }

        // Shrinking
        s_heapBreak += size;
        return p;
    }
    else
    {
        // TODO: do better
        Fatal("Out of memory");
    }
}


// TODO: make sure we don't start stepping over the heap!
void* vmm_allocate_pages(int pageCount)
{
    //Log("vmm_allocate_pages(%d)\n", pageCount);

    // TODO: need critical section here...

    // TODO: if allocating continuous frames fails, we should try in multiple chunks
    // TODO: error handling
    auto physicalAddress = pmm_allocate_frames(pageCount);

    // TODO: error handling
    void* virtualAddress = vmm_map_pages(physicalAddress, pageCount, PageType::KernelData_RW);

    // TODO: we should keep a pool of zero-ed memory
    // TODO: we don't always want to zero the memory! let the caller decide.
    memset(virtualAddress, 0, pageCount * MEMORY_PAGE_SIZE);

    return virtualAddress;
}


void vmm_free_pages(void* address, int pageCount)
{
    vmm_unmap_pages(address, pageCount);

    // TODO: free the (physical) memory!
}


void* arch_vmm_map_pages(physaddr_t physicalAddress, intptr_t pageCount, uint64_t flags)
{
    //Log("vmm_map_pages(%016llx, %d)\n", physicalAddress, pageCount);

    // TODO: need critical section here...
    assert(is_aligned(physicalAddress, MEMORY_PAGE_SIZE));

#if defined(__x86_64__)
    // Caller wants to map some physical memory but doesn't care about the virtual address.
    // Since we already mapped all physical memory, we can just return its virtual address.

//TODO: this sucks, we want to use this trick for hardware mapped devices as well, don't we?
//      the way to handle this is probably MTRRs, and they might already be setup properly by
//      the firmware... this needs to be verified.
    if (flags == (x86::PAGE_PRESENT | x86::PAGE_WRITE | x86::PAGE_NX))
    {
        return (void*)(physicalAddress + (physaddr_t)VMA_PHYSICAL_MAP_START);
    }
#endif

    s_mmapBegin = advance_pointer(s_mmapBegin, -(pageCount * MEMORY_PAGE_SIZE));

    // TODO: verify return value
    arch_vmm_map_pages(physicalAddress, s_mmapBegin, pageCount, flags);

    return s_mmapBegin;
}
