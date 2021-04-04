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
#include <cstring>
#include <memory>
#include <kernel/pagetable.hpp>
#include <kernel/pmm.hpp>
#include <metal/cpu.hpp>
#include <metal/helpers.hpp>
#include <metal/log.hpp>

using namespace x86;


/*
    Virtual Memory Map (ia32)


    0x00000000 - 0xEFFFFFFF     User space (3840 MB)
    0xF0000000 - 0xFF7FEFFF     Kernel (248 MB)

    0xFF7FF000 - 0xFF7FFFFF     Page Mapping Level 3 (PDPT)
    0xFF800000 - 0xFFFFBFFF     Page Mapping Level 1 (Page Tables)
    0xFFFFC000 - 0xFFFFFFFF     Page Mapping Level 2 (Page Directories)


    3 levels, 2/9/9 bits

    PML3: 0xFF7FF000 to 0xFF7FFFFF - 0x4 entries (2 bits), shift = (32 - 2) = 30
    PML2: 0xFFFFC000 to 0xFFFFFFFF - 0x800 entries (11 bits), shift = (32 - 11) = 21
    PML1: 0xFF800000 to 0xFFFFBFFF - 0x100000 entries (20 bits), shift = (32 - 20) = 12

    long i3 = (address >> 30) & 0x3;
    long i2 = (address >> 21) & 0x7FF;
    long i1 = (address >> 12) & 0xFFFFF;
*/


// Where we can find the page tables in virtual memory
static uint64_t* const vmm_pml3 = (uint64_t*)0xFF7FF000;
static uint64_t* const vmm_pml2 = (uint64_t*)0xFFFFC000;
static uint64_t* const vmm_pml1 = (uint64_t*)0xFF800000;


physaddr_t vmm_get_physical_address(void* virtualAddress)
{
    auto va = (physaddr_t)virtualAddress;

    const int i2 = (va >> 21) & 0x7FF;
    const int i1 = (va >> 12) & 0xFFFFF;

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


void arch_vmm_map_pages(physaddr_t physicalAddress, const void* virtualAddress, intptr_t pageCount, uint64_t flags)
{
    //Log("vmm_map_pages(%016llx, %p, %d)\n", physicalAddress, virtualAddress, pageCount);

    // TODO: need critical section here...

    assert(is_aligned(physicalAddress, MEMORY_PAGE_SIZE));
    assert(is_aligned(virtualAddress, MEMORY_PAGE_SIZE));

    // TODO: we need to be smarter about large pages as they could be used in the middle of the requested mapping

    // TODO: MTRRs can interfere with large/huge pages and we need to handle this properly

    const bool useLargePages =
        (pageCount & 0x1FF) == 0
        && (physicalAddress & 0x1FFFFF) == 0
        && ((physaddr_t)virtualAddress & 0x1FFFFF) == 0;

    if (useLargePages)
    {
        pageCount >>= 9;
    }

    for (auto page = 0; page != pageCount; ++page)
    {
        uintptr_t addr = (uintptr_t)virtualAddress;

        const int i3 = (addr >> 30) & 0x3;
        const int i2 = (addr >> 21) & 0x7FF;
        const int i1 = (addr >> 12) & 0xFFFFF;

        const uint64_t kernelSpaceFlags = (i2 >= 1920 && i2 < 2044) ? PAGE_GLOBAL : 0;

        if (!(vmm_pml3[i3] & PAGE_PRESENT))
        {
            const physaddr_t frame = pmm_allocate_frames(1);
            // NOTE: make sure not to put PAGE_WRITE on this entry, it is not legal.
            //       Bochs will validate this and crash. QEMU ignores it.
            vmm_pml3[i3] = frame | PAGE_PRESENT | (flags & PAGE_USER);

            auto p = (char*)vmm_pml2 + (i3 << 12);
            x86_invlpg(p);

            memset(p, 0, MEMORY_PAGE_SIZE);

            //TODO: this new page directory needs to be "recurse-mapped" in PD #3 [1FC-1FE]
            assert(0);
        }

        if (useLargePages)
        {
            assert(!(vmm_pml2[i2] & PAGE_PRESENT));
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

    // TODO: need critical section here...
    // TODO: TLB shutdown (SMP)
    // TODO: need to update memory map region and track holes
    // TODO: check if we can free page tables (pml1, pml2, pml3)
    // TODO: need to handle large pages

    auto va = (physaddr_t)virtualAddress;
    int i1 = (va >> 12) & 0xFFFFF;

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
    auto pml3 = (uint64_t*)vmm_allocate_pages(5);
    if (!pml3)
    {
        // TODO: is this what we want?
        return nullptr;
    }

    auto pml2 = pml3 + 512;

    // TODO: CR3 is 32 bits, we need to allocate memory under 4 GB
    const auto cr3 = vmm_get_physical_address(pml3);
    assert(cr3 < MEM_4_GB);

    // Setup PML3
    // NOTE: make sure not to put PAGE_WRITE on these 4 entries, it is not legal.
    //       Bochs will validate this and crash. QEMU ignores it.
    pml3[0] = vmm_get_physical_address(pml2) | PAGE_PRESENT;
    pml3[1] = vmm_get_physical_address(pml2 + 512) | PAGE_PRESENT;
    pml3[2] = vmm_get_physical_address(pml2 + 1024) | PAGE_PRESENT;
    pml3[3] = vmm_get_physical_address(pml2 + 1536) | PAGE_PRESENT;

    // Copy kernel address space
    memcpy(pml2 + 1920, vmm_pml2 + 1920, 124 * sizeof(uint64_t));

    // TODO: temporary - copy framebuffer mapping at 0xE0000000
    memcpy(pml2 + 1792, vmm_pml2 + 1792, 128 * sizeof(uint64_t));

    // Setup recursive mapping
    pml2[2044] = pml3[0] | PAGE_WRITE;
    pml2[2045] = pml3[1] | PAGE_WRITE;
    pml2[2046] = pml3[2] | PAGE_WRITE;
    pml2[2047] = pml3[3] | PAGE_WRITE;

    // The current address space doesn't need the new one mapped anymore
    vmm_unmap_pages(pml3, 5);

    return std::make_shared<PageTable>(cr3);
}
