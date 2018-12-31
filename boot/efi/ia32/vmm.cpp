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


void vmm_init()
{
    // To keep things simple, we are going to identity-map memory up to 0xF0000000.
    // The kernel will be mapped at 0xF0000000.

    //TODO: we must figure out PAE
}


void vmm_enable()
{
    //asm volatile ("mov %0, %%cr3" : : "r"(pml3 or 2));
}


void vmm_map(physaddr_t physicalAddress, physaddr_t virtualAddress, size_t size)
{
    Log("vmm_map: %X --> %X (%p)\n", physicalAddress, virtualAddress, size);

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
    (void)physicalAddress;
    (void)virtualAddress;
    Log("    vmm_map_page: %X --> %X\n", physicalAddress, virtualAddress);
}
