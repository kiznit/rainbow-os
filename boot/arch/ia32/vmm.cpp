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

#include "vmm.hpp"
#include <inttypes.h>
#include <stdio.h>
#include "memory.hpp"


/*
    Virtual Memory Map (ia32)

    0x00000000 - 0xBFFFFFFF     User space
    0xC0000000 - 0xC0100000     Low Memory (ISA IO SPACE, BIOS)
    0xC1000000 - 0xC1400000     Kiznix Kernel
    0xE0000000 - 0xEFFFFFFF     Heap space (vmm_alloc)
    0xFF000000 - 0xFF7FFFFF     Free memory pages stack (8 MB)

    Non-PAE:
    0xFFC00000 - 0xFFFFEFFF     Page Mapping Level 1 (Page Tables)
    0xFFFFF000 - 0xFFFFFFFF     Page Mapping Level 2 (Page Directory)

    PAE:
    0xFF800000 - 0xFFFFFFFF     Page Mappings
*/


//static uint32_t pageTable[1024] __attribute__((aligned(MEMORY_PAGE_SIZE))); // Root of page tables (Page Directory)



bool vmm_init()
{
    return true;
}



bool vmm_map_page(physaddr_t physicalAddress, physaddr_t virtualAddress)
{
    (void)physicalAddress;
    (void)virtualAddress;
    printf("    VMM_MAP_PAGE: %016" PRIx64 " --> %016" PRIx64 "\n", physicalAddress, virtualAddress);

    // const auto addr = (physaddr_t)virtualAddress;

    // const long i4 = (addr >> 39) & 0x1FF;
    // const long i3 = (addr >> 30) & 0x3FFFF;
    // const long i2 = (addr >> 21) & 0x7FFFFFF;
    // const long i1 = (addr >> 12) & 0xFFFFFFFFFul;

    // auto pml4 = (physaddr_t*) s_rootPageTable;

    // if (!(pml4[i4] & PAGE_PRESENT))
    // {
    //     const physaddr_t page = pmm_alloc_page();
    //     memset((void*)page, 0, MEMORY_PAGE_SIZE);
    //     pml4[i4] = page | PAGE_WRITE | PAGE_PRESENT;
    // }

    return true;
}



bool vmm_map(physaddr_t physicalAddress, physaddr_t virtualAddress, size_t size)
{
    printf("VMM_MAP: %016" PRIx64 " --> %016" PRIx64 " (%08lx)\n", physicalAddress, virtualAddress, size);

    size = align_up(size, MEMORY_PAGE_SIZE);

    while (size > 0)
    {
        vmm_map_page(physicalAddress, virtualAddress);
        size -= MEMORY_PAGE_SIZE;
        physicalAddress += MEMORY_PAGE_SIZE;
        virtualAddress = align_up(virtualAddress, MEMORY_PAGE_SIZE);
    }

    return true;
}
