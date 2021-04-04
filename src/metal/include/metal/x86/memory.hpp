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

#include <cstdint>


/*
    Intel Page Mapping Overview

    Pages are 4 KB (12 bits per page table entry)

    Page Table Level    x86         x86 PAE     x86_64          Intel Name
    ---------------------------------------------------------------------------------------------------
            4              -           -        9 bits          Page Mapping Level 4
            3              -         2 bits     9 bits          Page Directory Pointer Table
            2           10 bits      9 bits     9 bits          Page Directory
            1           10 bits      9 bits     9 bits          Page Table
         (page)         12 bits     12 bits    12 bits          Page
    ---------------------------------------------------------------------------------------------------
                        32 bits     32 bits    48 bits          Virtual address size
                        32 bits     36 bits    48 bits          Physical address size
                         4 GB        64 GB      256 TB          Addressable Physical Memory
*/



typedef uint64_t physaddr_t;


// Normal pages are 4 KB
constexpr auto MEMORY_PAGE_SHIFT = 12;
constexpr auto MEMORY_PAGE_SIZE = 4096;

// Large pages are 2 MB
constexpr auto MEMORY_LARGE_PAGE_SHIFT = 21;
constexpr auto MEMORY_LARGE_PAGE_SIZE = 2 * 1024 * 1024;

// Huge pages are 1 GB
constexpr auto MEMORY_HUGE_PAGE_SHIFT = 30;
constexpr auto MEMORY_HUGE_PAGE_SIZE = 1024 * 1024 * 1024;


namespace x86
{
    // Page mapping flags (12 bits)
    constexpr uint64_t PAGE_PRESENT         = 0x001;
    constexpr uint64_t PAGE_WRITE           = 0x002;
    constexpr uint64_t PAGE_USER            = 0x004;
    constexpr uint64_t PAGE_WRITE_THROUGH   = 0x008;
    constexpr uint64_t PAGE_CACHE_DISABLE   = 0x010;
    constexpr uint64_t PAGE_ACCESSED        = 0x020;
    constexpr uint64_t PAGE_DIRTY           = 0x040;
    constexpr uint64_t PAGE_SIZE            = 0x080;    // For page tables. If 0, entry is a page table otherwise it is a "large page" (similar to ARM memory blocks)

    // TODO: bad name, we need something meaningful here
    constexpr uint64_t PAGE_PAT             = 0x008;    // For page entries

    constexpr uint64_t PAGE_GLOBAL          = 0x100;
    constexpr uint64_t PAGE_RESERVED_0      = 0x200;    // Usable by OS
    constexpr uint64_t PAGE_RESERVED_1      = 0x400;    // Usable by OS
    constexpr uint64_t PAGE_RESERVED_2      = 0x800;    // Usable by OS

    // bits 52-62 are reserved for software use

    constexpr uint64_t PAGE_NX              = 1ull << 63;

    constexpr uint64_t PAGE_ADDRESS_MASK    = 0x000FFFFFFFFFF000ull;


    // Page fault flags
    constexpr uint64_t PAGEFAULT_PRESENT        = 0x01; // Page was not present
    constexpr uint64_t PAGEFAULT_WRITE          = 0x02; // A write access triggered the page fault
    constexpr uint64_t PAGEFAULT_USER           = 0x04; // A user mode access triggered the page fault
    constexpr uint64_t PAGEFAULT_RESERVED       = 0x08; // Page is reserved
    constexpr uint64_t PAGEFAULT_INSTRUCTION    = 0x10; // An instruction triggered the page fault
    constexpr uint64_t PAGEFAULT_PROTECTION_KEY = 0x20; // Address is protected by a key


    // PAT memory types
    constexpr uint64_t PAT_UNCACHEABLE          = 0x00; // Strong Ordering
    constexpr uint64_t PAT_WRITE_COMBINING      = 0x01; // Weak Ordering
    constexpr uint64_t PAT_WRITE_THROUGH        = 0x04; // Speculative Processor Ordering
    constexpr uint64_t PAT_WRITE_PROTECTED      = 0x05; // Speculative Processor Ordering
    constexpr uint64_t PAT_WRITE_BACK           = 0x06; // Speculative Processor Ordering
    constexpr uint64_t PAT_UNCACHEABLE_WEAK     = 0x07; // Strong Ordering, can be overridden by WC in MTRRs
} // namespace x86


constexpr inline physaddr_t GetPageFlags(PageType type)
{
    switch (type)
    {
        case PageType::KernelCode:          return x86::PAGE_PRESENT;
        case PageType::KernelData_RO:       return x86::PAGE_PRESENT | x86::PAGE_NX;
        case PageType::KernelData_RW:       return x86::PAGE_PRESENT | x86::PAGE_NX |                  x86::PAGE_WRITE;
        case PageType::UserCode:            return x86::PAGE_PRESENT |                x86::PAGE_USER;
        case PageType::UserData_RO:         return x86::PAGE_PRESENT | x86::PAGE_NX | x86::PAGE_USER;
        case PageType::UserData_RW:         return x86::PAGE_PRESENT | x86::PAGE_NX | x86::PAGE_USER | x86::PAGE_WRITE;
        case PageType::MMIO:                return x86::PAGE_PRESENT | x86::PAGE_NX |                  x86::PAGE_WRITE | x86::PAGE_WRITE_THROUGH | x86::PAGE_CACHE_DISABLE;
        case PageType::VideoFramebuffer:    return x86::PAGE_PRESENT | x86::PAGE_NX |                  x86::PAGE_WRITE | x86::PAGE_PAT;
    }

    __builtin_unreachable();
}


#endif
