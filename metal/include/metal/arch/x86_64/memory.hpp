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
    static constexpr auto MEMORY_PAGE_SHIFT = 12;
    static constexpr auto MEMORY_PAGE_SIZE = 4096;

    // Large pages are 2 MB
    static constexpr auto MEMORY_LARGE_PAGE_SHIFT = 21;
    static constexpr auto MEMORY_LARGE_PAGE_SIZE = 2 * 1024 * 1024;

    // Huge pages are 1 GB
    static constexpr auto MEMORY_HUGE_PAGE_SHIFT = 30;
    static constexpr auto MEMORY_HUGE_PAGE_SIZE = 1024 * 1024 * 1024;

} // namespace mtl
