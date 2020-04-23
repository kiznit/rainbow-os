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

#include <kernel/kernel.hpp>


/*
    Virtual Memory Map (x86_64)


    0x00000000 00000000 - 0x00007FFF FFFFFFFF   User space (128 TB)

    0xFFFF8000 00000000 - 0xFFFFFEFF FFFFFFFF   Free (127 TB)

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


bool PageTable::CloneKernelSpace()
{
    auto pml4 = (uint64_t*)g_vmm->AllocatePages(1);
    if (!pml4) return false;

    cr3 = GetPhysicalAddress(pml4);

    // Initialize address space below the kernel
    memset(pml4, 0, 510 * sizeof(uint64_t));

    // Copy kernel address space
    pml4[511] = vmm_pml4[511];

    // TODO: temporary - copy framebuffer mapping at 0xFFFF8000 00000000
    pml4[256] = vmm_pml4[256];

    // Setup recursive mapping
    pml4[510] = cr3 | PAGE_WRITE | PAGE_PRESENT;

    // The current address space doesn't need the new one mapped anymore
    UnmapPage(pml4);

    return true;
}


void PageTable::Enable(const PageTable& current)
{
    // TODO: right now this is flushing the entirety of the TLB, not good for performances

    if (cr3 != current.cr3)
    {
        x86_set_cr3(cr3);
    }
}


physaddr_t PageTable::GetPhysicalAddress(void* virtualAddress) const
{
    // TODO: this needs to take into account large pages
    auto va = (physaddr_t)virtualAddress;
    const long i1 = (va >> 12) & 0xFFFFFFFFFul;
    auto pa = vmm_pml1[i1] & PAGE_ADDRESS_MASK;

    return pa;
}


int PageTable::MapPages(physaddr_t physicalAddress, const void* virtualAddress, size_t pageCount, physaddr_t flags)
{
    for (size_t page = 0; page != pageCount; ++page)
    {
        //Log("MapPage: %X -> %p, %X\n", physicalAddress, virtualAddress, flags);

        uintptr_t addr = (uintptr_t)virtualAddress;

        const long i4 = (addr >> 39) & 0x1FF;
        const long i3 = (addr >> 30) & 0x3FFFF;
        const long i2 = (addr >> 21) & 0x7FFFFFF;
        const long i1 = (addr >> 12) & 0xFFFFFFFFFul;

        const uint64_t kernelSpaceFlags = (i4 == 0x1ff) ? PAGE_GLOBAL : 0;

        if (!(vmm_pml4[i4] & PAGE_PRESENT))
        {
            const physaddr_t page = g_pmm->AllocatePages(1);
            vmm_pml4[i4] = page | PAGE_WRITE | PAGE_PRESENT | kernelSpaceFlags | (flags & PAGE_USER);

            auto p = (char*)vmm_pml3 + (i4 << 12);
            vmm_invalidate(p);

            memset(p, 0, MEMORY_PAGE_SIZE);
        }

        if (!(vmm_pml3[i3] & PAGE_PRESENT))
        {
            const physaddr_t page = g_pmm->AllocatePages(1);
            vmm_pml3[i3] = page | PAGE_WRITE | PAGE_PRESENT | kernelSpaceFlags | (flags & PAGE_USER);

            auto p = (char*)vmm_pml2 + (i3 << 12);
            vmm_invalidate(p);

            memset(p, 0, MEMORY_PAGE_SIZE);
        }

        if (!(vmm_pml2[i2] & PAGE_PRESENT))
        {
            const physaddr_t page = g_pmm->AllocatePages(1);
            vmm_pml2[i2] = page | PAGE_WRITE | PAGE_PRESENT | kernelSpaceFlags | (flags & PAGE_USER);

            auto p = (char*)vmm_pml1 + (i2 << 12);
            vmm_invalidate(p);

            memset(p, 0, MEMORY_PAGE_SIZE);
        }

        assert(!(vmm_pml1[i1] & PAGE_PRESENT));

        vmm_pml1[i1] = physicalAddress | flags | kernelSpaceFlags;
        vmm_invalidate(virtualAddress);

        // Next page...
        physicalAddress += MEMORY_PAGE_SIZE;
        virtualAddress = advance_pointer(virtualAddress, MEMORY_PAGE_SIZE);
    }

    return 0;
}


void PageTable::UnmapPage(void* virtualAddress)
{
    // TODO: need to update memory map region and track holes
    // TODO: check if we can free page tables (pml1, pml2, pml3)

    auto va = (physaddr_t)virtualAddress;
    const long i1 = (va >> 12) & 0xFFFFFFFFFul;

    if (vmm_pml1[i1] & PAGE_PRESENT) // TODO: should be an assert?
    {
        vmm_pml1[i1] = 0;
    }
}
