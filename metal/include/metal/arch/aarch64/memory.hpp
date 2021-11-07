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
    static constexpr auto MEMORY_PAGE_SHIFT = 12;
    static constexpr auto MEMORY_PAGE_SIZE = 4096;

    // Large pages are 2 MB
    static constexpr auto MEMORY_LARGE_PAGE_SHIFT = 21;
    static constexpr auto MEMORY_LARGE_PAGE_SIZE = 2 * 1024 * 1024;

    // Huge pages are 1 GB
    static constexpr auto MEMORY_HUGE_PAGE_SHIFT = 30;
    static constexpr auto MEMORY_HUGE_PAGE_SIZE = 1024 * 1024 * 1024;

} // namespace mtl
