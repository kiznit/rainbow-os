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


bool PageTable::CloneKernelSpace()
{
    auto pml3 = (uint64_t*)vmm_allocate_pages(5);
    if (!pml3) return false;

    auto pml2 = pml3 + 512;

    cr3 = GetPhysicalAddress(pml3);

    // Setup PML3
    // NOTE: make sure not to put PAGE_WRITE on these 4 entries, it is not legal.
    //       Bochs will validate this and crash. QEMU ignores it.
    pml3[0] = GetPhysicalAddress(pml2) | PAGE_PRESENT;
    pml3[1] = GetPhysicalAddress(pml2 + 512) | PAGE_PRESENT;
    pml3[2] = GetPhysicalAddress(pml2 + 1024) | PAGE_PRESENT;
    pml3[3] = GetPhysicalAddress(pml2 + 1536) | PAGE_PRESENT;

    // Initialize address space below the kernel
    memset(pml2, 0, 1920 * sizeof(uint64_t));

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
    // TODO: provide APi to unmap consecutive pages
    UnmapPage(pml3);
    UnmapPage(pml3 + 512);
    UnmapPage(pml3 + 1024);
    UnmapPage(pml3 + 1536);
    UnmapPage(pml3 + 2048);

    return true;
}


physaddr_t PageTable::GetPhysicalAddress(void* virtualAddress) const
{
    // TODO: this needs to take into account large pages
    auto va = (physaddr_t)virtualAddress;
    const int i1 = (va >> 12) & 0xFFFFF;
    auto pa = vmm_pml1[i1] & PAGE_ADDRESS_MASK;

    return pa;
}


int PageTable::MapPages(physaddr_t physicalAddress, const void* virtualAddress, size_t pageCount, physaddr_t flags)
{
    for (size_t page = 0; page != pageCount; ++page)
    {
        //Log("MapPage: %X -> %p, %X\n", physicalAddress, virtualAddress, flags);

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

        if (!(vmm_pml2[i2] & PAGE_PRESENT))
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

    return 0;
}


void PageTable::UnmapPage(void* virtualAddress)
{
    // TODO: need to update memory map region and track holes
    // TODO: check if we can free page tables (pml1, pml2, pml3)

    auto va = (physaddr_t)virtualAddress;
    const int i1 = (va >> 12) & 0xFFFFF;

    if (vmm_pml1[i1] & PAGE_PRESENT) // TODO: should be an assert?
    {
        vmm_pml1[i1] = 0;
    }
}
