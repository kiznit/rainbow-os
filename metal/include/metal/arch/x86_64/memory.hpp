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
        // Page mapping flags (12 bits)
        Present         = 0x001,
        Write           = 0x002,
        User            = 0x004,
        WriteThrough    = 0x008,
        CacheDisable    = 0x010,
        Accessed        = 0x020,
        Dirty           = 0x040,
        Size            = 0x080,    // For page tables. If 0, entry is a page table otherwise it is a "large page" (similar to ARM memory blocks)

        // TODO: bad name, we need something meaningful here
        PAT             = 0x008,    // For page entries

        Global          = 0x100,
        Reserved0       = 0x200,    // Usable by OS
        Reserved1       = 0x400,    // Usable by OS
        Reserved2       = 0x800,    // Usable by OS

        // bits 52-62 are reserved for software use

        NX              = 1ull << 63,

        AddressMask     = 0x000FFFFFFFFFF000ull
    };

    enum class PageType : uint64_t
    {
        KernelCode          = PageFlags::Present,
        KernelData_RO       = PageFlags::Present | PageFlags::NX,
        KernelData_RW       = PageFlags::Present | PageFlags::NX |                   PageFlags::Write,
        UserCode            = PageFlags::Present |                 PageFlags::User,
        UserData_RO         = PageFlags::Present | PageFlags::NX | PageFlags::User,
        UserData_RW         = PageFlags::Present | PageFlags::NX | PageFlags::User | PageFlags::Write,
        //TODO: MMIO                = PageFlags::Present | PageFlags::NX |                   PageFlags::Write | PageFlags::WriteThrough | PageFlags::CacheDisable,
        //TODO: VideoFrameBuffer    = PageFlags::Present | PageFlags::NX |                   PageFlags::Write | PageFlags::PAT,
    };

    // clang-format on

} // namespace mtl
