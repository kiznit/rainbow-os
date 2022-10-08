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

    using PhysicalAddress = uint64_t;

    // Normal pages are 4 KB
    static constexpr auto kMemoryPageShift = 12;
    static constexpr auto kMemoryPageSize = 4096;

    // Large pages are 2 MB
    static constexpr auto kMemoryLargePageShift = 21;
    static constexpr auto kMemoryLargePageSize = 2 * 1024 * 1024;

    // Huge pages are 1 GB
    static constexpr auto kMemoryHugePageShift = 30;
    static constexpr auto kMemoryHugePageSize = 1024 * 1024 * 1024;

    // clang-format off

    enum PageFlags : uint64_t
    {
        // Page mapping flags (12 bits)
        Present         = 0x001,
        Write           = 0x002,
        User            = 0x004,
        //WriteThrough    = 0x008,
        CacheDisable    = 0x010,
        Accessed        = 0x020,
        Dirty           = 0x040,
        Size            = 0x080,    // For page tables. If 0, entry is a page table otherwise it is a "large page" (similar to ARM memory blocks)

        // Page Attribute Table
        WriteBack       = 0x000,    // PAT index 0
        WriteThrough    = 0x008,    // PAT index 1
        UncacheableWeak = 0x010,    // PAT index 2
        Uncacheable     = 0x018,    // PAT index 3
        WriteCombining  = 0x080,    // PAT index 4
        PAT_5           = 0x088,    // PAT index 5
        PAT_6           = 0x090,    // PAT index 6
        PAT_7           = 0x098,    // PAT index 7
        CacheMask       = 0x098,    // PAT index mask

        Global          = 0x100,
        Reserved0       = 0x200,    // Usable by OS
        Reserved1       = 0x400,    // Usable by OS
        Reserved2       = 0x800,    // Usable by OS

        // Bits 12..51 are the address mask
        AddressMask     = 0x000FFFFFFFFFF000ull,

        // Bits 52..62 are reserved for software use

        NX              = 1ull << 63,

        FlagsMask       = ~AddressMask & ~Accessed & ~Dirty,

        // Page types
        KernelCode          = Present                     | WriteBack,
        KernelData_RO       = Present | NX                | WriteBack,
        KernelData_RW       = Present | NX |        Write | WriteBack,
        UserCode            = Present |      User         | WriteBack,
        UserData_RO         = Present | NX | User         | WriteBack,
        UserData_RW         = Present | NX | User | Write | WriteBack,
        MMIO                = Present | NX |        Write | Uncacheable,
        VideoFrameBuffer    = Present | NX |        Write | WriteCombining,
    };

    // PAT Memory Types
    enum Pat : uint64_t
    {
        PatUncacheable      = 0x00, // Strong Ordering
        PatWriteCombining   = 0x01, // Weak Ordering
        PatWriteThrough     = 0x04, // Speculative Processor Ordering
        PatWriteProtected   = 0x05, // Speculative Processor Ordering
        PatWriteBack        = 0x06, // Speculative Processor Ordering
        PatUncacheableWeak  = 0x07, // Strong Ordering, can be overridden by WC in MTRRs
    };

    // clang-format on

} // namespace mtl
