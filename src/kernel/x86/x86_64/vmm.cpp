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

#include <kernel/vmm.hpp>
#include <cassert>
#include <cstring>
#include <memory>
#include <kernel/config.hpp>
#include <kernel/pagetable.hpp>
#include <kernel/pmm.hpp>
#include <metal/helpers.hpp>
#include <metal/log.hpp>
#include <metal/x86/cpuid.hpp>


/*
    Virtual Memory Map (x86_64)


    0x00000000 00000000 - 0x00007FFF FFFFFFFF   User space (128 TB)

    0xFFFF8000 00000000 - 0xFFFFBFFF FFFFFFFF   Direct mapping of all physical memory (64 TB)

    0xFFFFC000 00000000 - 0xFFFFFEFF FFFFFFFF   Free (63 TB)

    0xFFFFFF00 00000000 - 0xFFFFFF7F FFFFFFFF   Page Mapping Level 1 (Page Tables)
    0xFFFFFF7F 80000000 - 0xFFFFFF7F BFFFFFFF   Page Mapping Level 2 (Page Directories)
    0xFFFFFF7F BFC00000 - 0xFFFFFF7F BFDFFFFF   Page Mapping Level 3 (PDPTs / Page-Directory-Pointer Tables)
    0xFFFFFF7F BFDFE000 - 0xFFFFFF7F BFDFEFFF   Page Mapping Level 4 (PML4)

    0xFFFFFF80 00000000 - 0xFFFFFFFF 7FFFFFFF   Free (510 GB)
    0xFFFFFFFF 80000000 - 0xFFFFFFFF FFFFFFFF   Kernel (2 GB)


    4 levels, 9 bits each

    PML4: 0xFFFFFF7F BFDFE000 to 0xFFFFFF7F BFDFEFFF - 0x200 entries (9 bits), shift = (48 - 9) = 39
    PML3: 0xFFFFFF7F BFC00000 to 0xFFFFFF7F BFDFFFFF - 0x40000 entries (18 bits), shift = (48 - 18) = 30
    PML2: 0xFFFFFF7F 80000000 to 0xFFFFFF7F BFFFFFFF - 0x8000000 entries (27 bits), shift = (48 - 27) = 21
    PML1: 0xFFFFFF00 00000000 to 0xFFFFFF7F FFFFFFFF - 0x1000000000 entries (36 bits), shift = (48 - 36) = 12

    long i4 = (address >> 39) & 1FF;
    long i3 = (address >> 30) & 0x3FFFF;
    long i2 = (address >> 21) & 0x7FFFFFF;
    long i1 = (address >> 12) & 0xFFFFFFFFF;
*/


// Where we can find the page tables in virtual memory
static uint64_t* const vmm_pml4 = (uint64_t*)0xFFFFFF7FBFDFE000ull;
static uint64_t* const vmm_pml3 = (uint64_t*)0xFFFFFF7FBFC00000ull;
static uint64_t* const vmm_pml2 = (uint64_t*)0xFFFFFF7F80000000ull;
static uint64_t* const vmm_pml1 = (uint64_t*)0xFFFFFF0000000000ull;

static const bool s_hasHugePages = cpuid_has_1gbpage();


physaddr_t vmm_get_physical_address(void* virtualAddress)
{
    // Check if the address is within the range where we mapped all physical memory
    if (virtualAddress >= VMA_PHYSICAL_MAP_START && virtualAddress <= VMA_PHYSICAL_MAP_END)
    {
        return (physaddr_t)virtualAddress - (physaddr_t)VMA_PHYSICAL_MAP_START;
    }

    auto va = (physaddr_t)virtualAddress;

    const long i3 = (va >> 30) & 0x3FFFF;
    const long i2 = (va >> 21) & 0x7FFFFFF;
    const long i1 = (va >> 12) & 0xFFFFFFFFFul;

    if (s_hasHugePages)
    {
        if (vmm_pml3[i3] & PAGE_SIZE)
        {
            // Huge page
            auto offset = va & 0x3FFFFFFF;
            auto pa = (vmm_pml3[i3] & PAGE_ADDRESS_MASK) + offset;
            return pa;
        }
    }

    if (vmm_pml2[i2] & PAGE_SIZE)
    {
        // Large page
        auto offset = va & 0x1FFFFF;
        auto pa = (vmm_pml2[i2] & PAGE_ADDRESS_MASK) + offset;
        return pa;
    }

    // Normal page
    auto offset = va & 0xFFF;
    auto pa = (vmm_pml1[i1] & PAGE_ADDRESS_MASK) + offset;
    return pa;
}


void vmm_map_pages(physaddr_t physicalAddress, const void* virtualAddress, intptr_t pageCount, uint64_t flags)
{
    //Log("vmm_map_pages(%016llx, %p, %d)\n", physicalAddress, virtualAddress, pageCount);

    // TODO: need critical section here...

    assert(is_aligned(physicalAddress, MEMORY_PAGE_SIZE));
    assert(is_aligned(virtualAddress, MEMORY_PAGE_SIZE));

    // TODO: we need to be smarter about large/huge pages as they could be used in the middle of the requested mapping

    // TODO: MTRRs can interfere with large/huge pages and we need to handle this properly

    const bool useHugePages = s_hasHugePages
        && (pageCount & 0x3FFFF) == 0
        && (physicalAddress & 0x3FFFFFFF) == 0
        && ((physaddr_t)virtualAddress & 0x3FFFFFFF) == 0;

    const bool useLargePages = !useHugePages
        && (pageCount & 0x1FF) == 0
        && (physicalAddress & 0x1FFFFF) == 0
        && ((physaddr_t)virtualAddress & 0x1FFFFF) == 0;

    if (useHugePages)
    {
        pageCount >>= 18;
    }
    else if (useLargePages)
    {
        pageCount >>= 9;
    }

    for (auto page = 0; page != pageCount; ++page)
    {
        uintptr_t addr = (uintptr_t)virtualAddress;

        const long i4 = (addr >> 39) & 0x1FF;
        const long i3 = (addr >> 30) & 0x3FFFF;
        const long i2 = (addr >> 21) & 0x7FFFFFF;
        const long i1 = (addr >> 12) & 0xFFFFFFFFFul;

        const uint64_t kernelSpaceFlags = (i4 == 0x1ff) ? PAGE_GLOBAL : 0;

        if (!(vmm_pml4[i4] & PAGE_PRESENT))
        {
            const physaddr_t frame = pmm_allocate_frames(1);
            vmm_pml4[i4] = frame | PAGE_WRITE | PAGE_PRESENT | kernelSpaceFlags | (flags & PAGE_USER);

            auto p = (char*)vmm_pml3 + (i4 << 12);
            x86_invlpg(p);

            memset(p, 0, MEMORY_PAGE_SIZE);
        }

        if (useHugePages)
        {
            assert(!vmm_pml3[i3] & PAGE_PRESENT);
            vmm_pml3[i3] = physicalAddress | flags | kernelSpaceFlags | PAGE_SIZE;
            x86_invlpg(virtualAddress);

            // Next page...
            physicalAddress += MEMORY_HUGE_PAGE_SIZE;
            virtualAddress = advance_pointer(virtualAddress, MEMORY_HUGE_PAGE_SIZE);
            continue;
        }
        else if (!(vmm_pml3[i3] & PAGE_PRESENT))
        {
            const physaddr_t frame = pmm_allocate_frames(1);
            vmm_pml3[i3] = frame | PAGE_WRITE | PAGE_PRESENT | kernelSpaceFlags | (flags & PAGE_USER);

            auto p = (char*)vmm_pml2 + (i3 << 12);
            x86_invlpg(p);

            memset(p, 0, MEMORY_PAGE_SIZE);
        }

        if (useLargePages)
        {
            assert(!vmm_pml2[i2] & PAGE_PRESENT);
            vmm_pml2[i2] = physicalAddress | flags | kernelSpaceFlags | PAGE_SIZE;
            x86_invlpg(virtualAddress);

            // Next page...
            physicalAddress += MEMORY_LARGE_PAGE_SIZE;
            virtualAddress = advance_pointer(virtualAddress, MEMORY_LARGE_PAGE_SIZE);
            continue;
        }
        else if (!(vmm_pml2[i2] & PAGE_PRESENT))
        {
            const physaddr_t frame = pmm_allocate_frames(1);
            vmm_pml2[i2] = frame | PAGE_WRITE | PAGE_PRESENT | kernelSpaceFlags | (flags & PAGE_USER);

            auto p = (char*)vmm_pml1 + (i2 << 12);
            x86_invlpg(p);

            memset(p, 0, MEMORY_PAGE_SIZE);
        }

        assert(!(vmm_pml1[i1] & PAGE_PRESENT));

        vmm_pml1[i1] = physicalAddress | flags | kernelSpaceFlags;
        x86_invlpg(virtualAddress);

        // Next page...
        physicalAddress += MEMORY_PAGE_SIZE;
        virtualAddress = advance_pointer(virtualAddress, MEMORY_PAGE_SIZE);
    }
}


void vmm_unmap_pages(const void* virtualAddress, int pageCount)
{
    assert(is_aligned(virtualAddress, MEMORY_PAGE_SIZE));
    assert(virtualAddress < VMA_PHYSICAL_MAP_START || virtualAddress > VMA_PHYSICAL_MAP_START);

    // TODO: need critical section here...
    // TODO: TLB shutdown (SMP)
    // TODO: need to update memory map region and track holes
    // TODO: check if we can free page tables (pml1, pml2, pml3)
    // TODO: need to handle large and huge pages

    auto va = (physaddr_t)virtualAddress;
    long i1 = (va >> 12) & 0xFFFFFFFFFul;

    for ( ; pageCount > 0; --pageCount, ++i1)
    {
        if (vmm_pml1[i1] & PAGE_PRESENT) // TODO: should be an assert?
        {
            vmm_pml1[i1] = 0;
        }
    }
}


std::shared_ptr<PageTable> PageTable::CloneKernelSpace()
{
    auto pml4 = (uint64_t*)vmm_allocate_pages(1);
    if (!pml4)
    {
        // TODO: is this what we want?
        return nullptr;
    }

    // TODO: SMP trampoline code needs CR3 to be under 4 GB, this is because
    // it temporarely runs in 32 bits protected mode and CR3 is 32 bits there.
    // Can we figure out a way to ask for this explictly as it is not normally
    // a constraint?
    const auto cr3 = vmm_get_physical_address(pml4);
    assert(cr3 < MEM_4_GB);

    // Copy kernel address space
    memcpy(pml4 + 256, vmm_pml4 + 256, 256 * sizeof(uint64_t));

    // Setup recursive mapping
    pml4[510] = cr3 | PAGE_WRITE | PAGE_PRESENT;

    // The current address space doesn't need the new one mapped anymore
    vmm_unmap_pages(pml4, 1);

    return std::make_shared<PageTable>(cr3);
}
