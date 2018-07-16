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
#include <string.h>
#include "boot.hpp"
#include "memory.hpp"

/*
    Virtual Memory Map (x86_64)

    0x00000000 00000000 - 0x00007FFF FFFFFFFF   User space
    0xFFFF8000 00000000 - 0xFFFEFFFF FFFFFFFF   Unused kernel space
    0xFFFFF000 00000000 - 0xFFFFF07F FFFFFFFF   Free memory pages stack (512 GB)
    0xFFFFFF00 00000000 - 0xFFFFFF7F FFFFFFFF   Page Mapping Level 1 (Page Tables)
    0xFFFFFF7F 80000000 - 0xFFFFFF7F BFFFFFFF   Page Mapping Level 2 (Page Directories)
    0xFFFFFF7F BFC00000 - 0xFFFFFF7F BFDFFFFF   Page Mapping Level 3 (PDPTs / Page-Directory-Pointer Tables)
    0xFFFFFF7F BFDFE000 - 0xFFFFFF7F BFDFEFFF   Page Mapping Level 4 (PML4)
    0xFFFFFFFF C0000000 - 0xFFFFFFFF C0100000   Low Memory (ISA IO Space, BIOS, VGA, ...)
    0xFFFFFFFF C0100000 - 0xFFFFFFFF C0140000   Rainbow Kernel
    0xFFFFFFFF E0000000 - 0xFFFFFFFF EFFFFFFF   Heap space (vmm_alloc)
*/

/*
    Page tables (x86_64)

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


// We want to identiy-map the first 4MB of memory.
// We statically allocate the pages we need to do so.
static physaddr_t pml4[512] __attribute__((aligned(MEMORY_PAGE_SIZE)));
static physaddr_t pml3[512] __attribute__((aligned(MEMORY_PAGE_SIZE)));
static physaddr_t pml2[512] __attribute__((aligned(MEMORY_PAGE_SIZE)));
static physaddr_t pml1[1024] __attribute__((aligned(MEMORY_PAGE_SIZE)));


// static physaddr_t pmm_alloc_page()
// {
//     return g_memoryMap.AllocatePages(MemoryType_Kernel, 1);
// }



bool vmm_init()
{
    // Identity map the first 4 MB
    pml4[0] = (physaddr_t)pml3 | PAGE_WRITE | PAGE_PRESENT;

    pml3[0] = (physaddr_t)pml2 | PAGE_WRITE | PAGE_PRESENT;

    pml2[0] = (physaddr_t)pml1 | PAGE_WRITE | PAGE_PRESENT;
    pml2[1] = ((physaddr_t)pml1 + MEMORY_PAGE_SIZE) | PAGE_WRITE | PAGE_PRESENT;

    physaddr_t address = 0;
    for (int i = 0; i != 1024; ++i)
    {
        pml1[i] = address | PAGE_WRITE | PAGE_PRESENT;
        address += MEMORY_PAGE_SIZE;
    }

    return true;
}



bool vmm_map_page(physaddr_t physicalAddress, physaddr_t virtualAddress)
{
    (void)physicalAddress;
    (void)virtualAddress;
    //printf("    VMM_MAP_PAGE: %016" PRIx64 " --> %016" PRIx64 "\n", physicalAddress, virtualAddress);

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
    //printf("VMM_MAP: %016" PRIx64 " --> %016" PRIx64 " (%08lx)\n", physicalAddress, virtualAddress, size);

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
