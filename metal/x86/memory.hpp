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

#ifndef _RAINBOW_METAL_X86_MEMORY_HPP
#define _RAINBOW_METAL_X86_MEMORY_HPP

#include <stdint.h>


/*
    Intel Page Mapping Overview
    Pages are 4 KB (12 bits per page table entry)
    Page Table Level    x86         x86 PAE     x86_64          Intel Name
    ---------------------------------------------------------------------------------------------------
            4            -             -        9 bits          Page Mapping Level 4
            3            -           2 bits     9 bits          Page Directory Pointer Table
            2           10 bits      9 bits     9 bits          Page Directory
            1           10 bits      9 bits     9 bits          Page Table
         (page)         12 bits     12 bits    12 bits          Page Table Entries
    ---------------------------------------------------------------------------------------------------
                        32 bits     32 bits    48 bits
                         4 GB        64 GB      256 TB          Addressable Physical Memory
*/



// Memory
typedef uint64_t physaddr_t;

#define MEMORY_PAGE_SHIFT 12
#define MEMORY_PAGE_SIZE 4096


#if defined(__i386__)
#define MEMORY_LARGE_PAGE_SHIFT 21
#define MEMORY_LARGE_PAGE_SIZE (2*1024*1024)
#elif defined(__x86_64__)
#define MEMORY_LARGE_PAGE_SHIFT 22
#define MEMORY_LARGE_PAGE_SIZE (4*1024*1024)
#endif

// Huge pages are 1 GB
#define MEMORY_HUGE_PAGE_SHIFT 30
#define MEMORY_HUGE_PAGE_SIZE (1024*1024*1024)  // 1 GB

// Page mapping flags (12 bits)
#define PAGE_PRESENT        0x001
#define PAGE_WRITE          0x002
#define PAGE_USER           0x004
#define PAGE_WRITE_THROUGH  0x008
#define PAGE_CACHE_DISABLE  0x010
#define PAGE_ACCESSED       0x020
#define PAGE_DIRTY          0x040
#define PAGE_LARGE          0x080

#define PAGE_GLOBAL         0x100
#define PAGE_RESERVED_0     0x200   // Usable by OS
#define PAGE_RESERVED_1     0x040   // Usable by OS
#define PAGE_RESERVED_2     0x800   // Usable by OS

// bits 52-62 are also usable by the OS

#define PAGE_NX             (1ull << 63)

#define PAGE_ADDRESS_MASK   (0x000FFFFFFFFFF000ull)


// Page fault flags
#define PAGEFAULT_PRESENT           0x01    // Page was not present
#define PAGEFAULT_WRITE             0x02    // A write access triggered the page fault
#define PAGEFAULT_USER              0x04    // A user mode access triggered the page fault
#define PAGEFAULT_RESERVED          0x08    // Page is reserved
#define PAGEFAULT_INSTRUCTION       0x10    // An instruction triggered the page fault
#define PAGEFAULT_PROTECTION_KEY    0x20    // Address is protected by a key


static inline void vmm_invalidate(const void* virtualAddress)
{
    asm volatile ("invlpg (%0)" : : "r"(virtualAddress) : "memory");
}


#endif
