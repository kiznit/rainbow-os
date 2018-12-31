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
#include "boot.hpp"
#include "log.hpp"


physaddr_t pml4[512] __attribute__((aligned (MEMORY_PAGE_SIZE)));
physaddr_t pml3[512] __attribute__((aligned (MEMORY_PAGE_SIZE)));
physaddr_t pml2[2048] __attribute__((aligned (MEMORY_PAGE_SIZE)));


void vmm_init()
{
    // To keep things simple, we are going to identity-map the first 4 GB of memory.
    // The kernel will be mapped outside of the first 4GB of memory.

    // We are not going to trust the loader to clear BSS properly
    memset(pml4, 0, sizeof(pml4));
    memset(pml3, 0, sizeof(pml3));

    // 1 entry = 512 GB
    pml4[0] = (physaddr_t)pml3 | PAGE_WRITE | PAGE_PRESENT;

    // 4 entries = 4 x 1GB = 4 GB
    pml3[0] = (physaddr_t)&pml2[0] | PAGE_WRITE | PAGE_PRESENT;
    pml3[1] = (physaddr_t)&pml2[512] | PAGE_WRITE | PAGE_PRESENT;
    pml3[2] = (physaddr_t)&pml2[1024] | PAGE_WRITE | PAGE_PRESENT;
    pml3[3] = (physaddr_t)&pml2[1536] | PAGE_WRITE | PAGE_PRESENT;

    // 2048 entries = 2048 * 2 MB = 4 GB
    for (physaddr_t i = 0; i != 2048; ++i)
    {
        pml2[i] = i * 512 * MEMORY_PAGE_SIZE | PAGE_LARGE | PAGE_WRITE | PAGE_PRESENT;
    }
}



void vmm_enable()
{
    asm volatile ("mov %0, %%cr3" : : "r"(pml4));
}



void vmm_map(physaddr_t physicalAddress, physaddr_t virtualAddress, size_t size)
{
    size = align_up(size, MEMORY_PAGE_SIZE);

    while (size > 0)
    {
        vmm_map_page(physicalAddress, virtualAddress);
        size -= MEMORY_PAGE_SIZE;
        physicalAddress += MEMORY_PAGE_SIZE;
        virtualAddress += MEMORY_PAGE_SIZE;
    }
}



void vmm_map_page(physaddr_t physicalAddress, physaddr_t virtualAddress)
{
    const long i4 = (virtualAddress >> 39) & 0x1FF;
    const long i3 = (virtualAddress >> 30) & 0x1FF;
    const long i2 = (virtualAddress >> 21) & 0x1FF;
    const long i1 = (virtualAddress >> 12) & 0x1FF;

    if (!(pml4[i4] & PAGE_PRESENT))
    {
        const physaddr_t page = (physaddr_t)AllocatePages(1);
        pml4[i4] = page | PAGE_WRITE | PAGE_PRESENT;
        memset((void*)page, 0, MEMORY_PAGE_SIZE);
    }

    physaddr_t* pml3 = (physaddr_t*)(pml4[i4] & ~(MEMORY_PAGE_SIZE - 1));
    if (!(pml3[i3] & PAGE_PRESENT))
    {
        const physaddr_t page = (physaddr_t)AllocatePages(1);
        memset((void*)page, 0, MEMORY_PAGE_SIZE);
        pml3[i3] = page | PAGE_WRITE | PAGE_PRESENT;
    }

    physaddr_t* pml2 = (physaddr_t*)(pml3[i3] & ~(MEMORY_PAGE_SIZE - 1));
    if (!(pml2[i2] & PAGE_PRESENT))
    {
        const physaddr_t page = (physaddr_t)AllocatePages(1);
        memset((void*)page, 0, MEMORY_PAGE_SIZE);
        pml2[i2] = page | PAGE_WRITE | PAGE_PRESENT;
    }

    physaddr_t* pml1 = (physaddr_t*)(pml2[i2] & ~(MEMORY_PAGE_SIZE - 1));
    if (pml1[i1] & PAGE_PRESENT)
    {
        Fatal("vmm_map_page() - there is already something there! (i1 = %d, entry = %X)\n", i1, pml1[i1]);
    }

    pml1[i1] = physicalAddress | PAGE_WRITE | PAGE_PRESENT;
}
