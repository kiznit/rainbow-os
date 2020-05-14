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

#ifndef _RAINBOW_KERNEL_PAGETABLE_HPP
#define _RAINBOW_KERNEL_PAGETABLE_HPP

#include <stddef.h>
#include <metal/arch.hpp>


// This represents the hardware level page mapping. It is possible that
// some architectures don't actually use page tables in their implementation.
struct PageTable
{
    // Clone the current page table (kernel space only)
    bool CloneKernelSpace();

    // Return the physical address of the specified virtual memory address
    // Note: this is only going to work if the virtual address is mapped in the current page table!
    physaddr_t GetPhysicalAddress(void* virtualAddress) const;

    // Map the specified physical page to the specified virtual page
    // Returns 0 on success or an error code
    int MapPages(physaddr_t physicalAddress, const void* virtualAddress, size_t pageCount, physaddr_t flags);

    // Unmap the specified virtual memory page
    void UnmapPage(void* virtualAddress);

#if defined(__i386__) || defined(__x86_64__)
    uintptr_t cr3;
#endif
};


#endif
