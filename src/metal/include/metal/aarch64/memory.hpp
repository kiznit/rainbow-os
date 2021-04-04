/*
    Copyright (c) 2021, Thierry Tremblay
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

#ifndef _RAINBOW_METAL_AARCH64_MEMORY_HPP
#define _RAINBOW_METAL_AARCH64_MEMORY_HPP

#include <cstdint>


/*
    AArch64

      Page Table Level      Bits        ARM Name
      ---------------------------------------------------------------------------------------------------
              3            9 bits       Level 1 table (1 GB / entry)
              2            9 bits       Level 2 table (2 MB / entry)
              1            9 bits       Level 3 table (4 KB / entry)
           (page)         12 bits       Page
      ---------------------------------------------------------------------------------------------------
                          48 bits       Virtual address size
                          48 bits       Physical address size
                           256 TB       Addressable Physical Memory
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


namespace aarch64
{
    // bits 55-58 are reserved for software use
    constexpr uint64_t PAGE_UXN                 = 1ull << 54;   // Unpriviledge execute never
    constexpr uint64_t PAGE_PXN                 = 1ull << 53;   // Priviledge execute never
    constexpr uint64_t PAGE_CONTIGUOUS          = 1ull << 52;   // Optimization to efficiently use TLB space
    constexpr uint64_t PAGE_DBM                 = 1ull << 51;   // Dirty Bit Modifier
    constexpr uint64_t PAGE_AF                  = 1 << 10;      // Access flag (if 0, will trigger a page fault)
    constexpr uint64_t PAGE_SH                  = 3 << 8;       // Shareable
    constexpr uint64_t PAGE_AP2                 = 1 << 7;       // Read only (opposite of PAGE_WRITE on x86)
    constexpr uint64_t PAGE_AP1                 = 1 << 6;       // EL0 (user) access (aka PAGE_USER on x86)
    constexpr uint64_t PAGE_NS                  = 1 << 5;       // Security bit, but only at EL3 and Secure EL1
    constexpr uint64_t PAGE_Indx                = 7 << 2;       // Index into the MAIR_ELn (similar to x86 PATs)
    constexpr uint64_t PAGE_TABLE               = 1 << 1;       // Entry is a page table
    constexpr uint64_t PAGE_VALID               = 1 << 0;       // Page is valid (similar to P = Present on x86)

    // Aliases
    constexpr auto PAGE_USER     = PAGE_AP1;    // Accessible to user space
    constexpr auto PAGE_READONLY = PAGE_AP2;    // Read-only
} // namespace aarch64


constexpr inline physaddr_t GetPageFlags(PageType type)
{
    switch (type)
    {
        case PageType::KernelCode:          return aarch64::PAGE_VALID | aarch64::PAGE_UXN |                                          aarch64::PAGE_READONLY;
        case PageType::KernelData_RO:       return aarch64::PAGE_VALID | aarch64::PAGE_UXN | aarch64::PAGE_PXN |                      aarch64::PAGE_READONLY;
        case PageType::KernelData_RW:       return aarch64::PAGE_VALID | aarch64::PAGE_UXN | aarch64::PAGE_PXN;
        case PageType::UserCode:            return aarch64::PAGE_VALID |                                         aarch64::PAGE_USER | aarch64::PAGE_READONLY;
        case PageType::UserData_RO:         return aarch64::PAGE_VALID | aarch64::PAGE_UXN | aarch64::PAGE_PXN | aarch64::PAGE_USER | aarch64::PAGE_READONLY;
        case PageType::UserData_RW:         return aarch64::PAGE_VALID | aarch64::PAGE_UXN | aarch64::PAGE_PXN | aarch64::PAGE_USER;
        case PageType::MMIO:                return aarch64::PAGE_VALID | aarch64::PAGE_UXN | aarch64::PAGE_PXN; /* todo: disable caching */
        case PageType::VideoFramebuffer:    return aarch64::PAGE_VALID | aarch64::PAGE_UXN | aarch64::PAGE_PXN; /* todo: enable write-combining */
    }

    __builtin_unreachable();
}


#endif
