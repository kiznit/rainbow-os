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

#include <kernel/vmm.hpp>
#include <kernel/pmm.hpp>
#include <metal/arch.hpp>
#include <metal/log.hpp>
#include <rainbow/boot.hpp>


/*
    Virtual Memory Map (ia32, no PAE)


    0x00000000 - 0xEFFFFFFF     User space (3840 MB)
    0xF0000000 - 0xFFBFFFFF     Kernel (252 MB)
        0xFFBE0000 - 0xFFBFFFFF     Stack stack
    0xFFC00000 - 0xFFFFEFFF     Page Mapping Level 1 (Page Tables)
    0xFFFFF000 - 0xFFFFFFFF     Page Mapping Level 2 (Page Directory)
*/


/*
    2 levels, 10 bits each
    PML2: 0xFFFFF000 to 0xFFFFFFFF - 0x400 entries (10 bits), shift = (32 - 10) = 22
    PML1: 0xFFC00000 to 0xFFFFEFFF - 0x100000 entries (20 bits), shift = (32 - 20) = 12
    long i2 = (address >> 22) & 0x3FF;
    long i1 = (address >> 12) & 0xFFFFF;
*/


// Where we can find the page tables in virtual memory
static uint32_t* const vmm_pml2 = (uint32_t*)0xFFFFF000;
static uint32_t* const vmm_pml1 = (uint32_t*)0xFFC00000;



void vmm_init()
{
    Log("vmm_init  : check!\n");

    // We don't have anything to do here as we rely on the bootloader to setup recursive mapping properly.
}



int vmm_map_page(physaddr_t physicalAddress, void* virtualAddress)
{
    Log("vmm_map_page(%X, %p)\n", physicalAddress, virtualAddress);

    uintptr_t addr = (uintptr_t)virtualAddress;

    const int i2 = (addr >> 22) & 0x3FF;
    const int i1 = (addr >> 12) & 0xFFFFF;

    if (!(vmm_pml2[i2] & PAGE_PRESENT))
    {
        const physaddr_t page = pmm_allocate_pages(1);
        vmm_pml2[i2] = page | PAGE_WRITE | PAGE_PRESENT;

        auto p = (char*)vmm_pml1 + (i2 << 12);
        vmm_invalidate(p);

        memset(p, 0, MEMORY_PAGE_SIZE);
    }

    //todo: this should just be an assert
    if (vmm_pml1[i1] & PAGE_PRESENT)
    {
        Fatal("vmm_map_page() - there is already something there!");
    }

    vmm_pml1[i1] = physicalAddress | PAGE_WRITE | PAGE_PRESENT;
    vmm_invalidate(virtualAddress);

    return 0;
}
