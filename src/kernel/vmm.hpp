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

#ifndef _RAINBOW_KERNEL_VMM_HPP
#define _RAINBOW_KERNEL_VMM_HPP

#include <metal/arch.hpp>


// Initialize the virtual memory manager
void vmm_initialize();

// Allocate pages of memory and map them in kernel space
// All memory is committed right away.
// Note: pages will be zero-ed for you! Nice!
void* vmm_allocate_pages(int pageCount);

// Free pages
void vmm_free_pages(void* virtualAddress, int pageCount);

// Map physical pages
void* vmm_map_pages(physaddr_t physicalAddress, int pageCount, uint64_t flags);
void vmm_map_pages(physaddr_t physicalAddress, const void* virtualAddress, int pageCount, uint64_t flags);

// Unmap pages
void vmm_unmap_pages(const void* virtualAddress, int pageCount);

// Return the physical address of the specified virtual memory address
// Note: this is only going to work if the virtual address is mapped in the current page table!
physaddr_t vmm_get_physical_address(void* virtualAddress);

// Kernel heap management
void* vmm_sbrk(ptrdiff_t size);


#endif
