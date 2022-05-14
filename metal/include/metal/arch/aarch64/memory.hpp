/*
    Copyright (c) 2022, Thierry Tremblay
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

#pragma once

#include <cstdint>

namespace mtl
{
    /*
        AArch64 Page Mapping Overview

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

    using PhysicalAddress = uint64_t;

    // Normal pages are 4 KB
    inline constexpr auto MemoryPageShift = 12;
    inline constexpr auto MemoryPageSize = 4096;

    // Large pages are 2 MB
    inline constexpr auto MemoryLargePageShift = 21;
    inline constexpr auto MemoryLargePageSize = 2 * 1024 * 1024;

    // Huge pages are 1 GB
    inline constexpr auto MemoryHugePageShift = 30;
    inline constexpr auto MemoryHugePageSize = 1024 * 1024 * 1024;

    // clang-format off

    enum PageFlags : uint64_t
    {
        // bits 55-58 are reserved for software use
        UXN                 = 1ull << 54,   // Unpriviledge execute never
        PXN                 = 1ull << 53,   // Priviledge execute never
        Contiguous          = 1ull << 52,   // Optimization to efficiently use TLB space
        DirtyBitModifier    = 1ull << 51,   // Dirty Bit Modifier
        AccessFlag          = 1 << 10,      // Access flag (if 0, will trigger a page fault)
        Shareable           = 3 << 8,       // Shareable
        AP2                 = 1 << 7,       // Read only (opposite of PAGE_WRITE on x86)
        AP1                 = 1 << 6,       // EL0 (user) access (aka PAGE_USER on x86)
        NS                  = 1 << 5,       // Security bit, but only at EL3 and Secure EL1
        Index               = 7 << 2,       // Index into the MAIR_ELn (similar to x86 PATs)
        Table               = 1 << 1,       // Entry is a page table
        Valid               = 1 << 0,       // Page is valid (similar to P = Present on x86)

        //TODO :AddressMask         = 0x000FFFFFFFFFF000ull,

        // Aliases
        User                = AP1,          // Accessible to user space
        ReadOnly            = AP2,          // Read-only
    };

    enum class PageType : uint64_t
    {
        KernelCode          = PageFlags::Valid | PageFlags::UXN |                                    PageFlags::ReadOnly,
        KernelData_RO       = PageFlags::Valid | PageFlags::UXN | PageFlags::PXN |                   PageFlags::ReadOnly,
        KernelData_RW       = PageFlags::Valid | PageFlags::UXN | PageFlags::PXN,
        UserCode            = PageFlags::Valid |                                   PageFlags::User | PageFlags::ReadOnly,
        UserData_RO         = PageFlags::Valid | PageFlags::UXN | PageFlags::PXN | PageFlags::User | PageFlags::ReadOnly,
        UserData_RW         = PageFlags::Valid | PageFlags::UXN | PageFlags::PXN | PageFlags::User,
        //TODO: MMIO                = PageFlags::Valid | PageFlags::UXN | PageFlags::PXN, /* todo: disable caching */
        //TODO: VideoFramebuffer    = PageFlags::Valid | PageFlags::UXN | PageFlags::PXN, /* todo: enable write-combining */
    };

    // clang-format on

} // namespace mtl
