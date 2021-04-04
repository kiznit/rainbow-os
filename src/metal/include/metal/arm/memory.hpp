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

#ifndef _RAINBOW_METAL_ARM_MEMORY_HPP
#define _RAINBOW_METAL_ARM_MEMORY_HPP

#include <cstdint>


/*
    ARMv7 supports two different paging modes: short and long descriptors.

    Short-descriptor format

        - Entries are 32 bits
        - First level table size is (up to) 16 KB
        - Second level table size is 1 KB
        - Each entry in the first-level table represents 1 MB of VMA.
        - Optionally can access 40 bits of PA using supersections (at 16 MB granularity)

    Long-descriptor format

        - Entries are 64 bits
        - Each entry in the first-level table represents 1 GB of VMA.
        - Each entry in the second-level table represents 2 MB of VMA.


      Page Table Level    Short       Long          ARM Name
      ---------------------------------------------------------------------------------------------------
              3              -         2             -  / 1st level translation table
              2           12 bits      9 bits       1st / 2nd level translation table
              1            8 bits      9 bits       2nd / 3rd level translation table (aka Page Table)
           (page)         12 bits     12 bits       Page
      ---------------------------------------------------------------------------------------------------
                          32 bits     32 bits       Virtual address size
                          32 bits     40 bits       Physical address size
                            4 GB        1 TB        Addressable Physical Memory
*/


/*
namespace ShortDescriptor
{
    using Entry = uint32_t;
    using physaddr_t = uint32_t;

    // Normal pages are 4 KB
    constexpr auto MEMORY_PAGE_SHIFT = 12;
    constexpr auto MEMORY_PAGE_SIZE = 4096;

    // Large pages are 64 KB
    constexpr auto MEMORY_LARGE_PAGE_SHIFT = 16;
    constexpr auto MEMORY_LARGE_PAGE_SIZE =  64 * 1024;

    // Huge pages ("sections") are 1 MB
    constexpr auto MEMORY_HUGE_PAGE_SHIFT = 20;
    constexpr auto MEMORY_HUGE_PAGE_SIZE = 1024 * 1024;

    // Gigantic pages ("supersections") are 16 MB (support is optional)
    constexpr auto MEMORY_GIGANTIC_PAGE_SHIFT = 24;
    constexpr auto MEMORY_GIGANTIC_PAGE_SIZE = 16 * 1024 * 1024;

    // Short-format first-level translation table (PML2)
    //    - Can contain second-level tables for small or large pages
    //    - Can contain sections or supersections
    constexpr uint32_t PML2_TYPE_INVALID        = 0x00;
    constexpr uint32_t PML2_TYPE_PAGE_TABLE     = 0x01;
    constexpr uint32_t PML2_TYPE_SECTION        = 0x02;     // Huge pages
    constexpr uint32_t PML2_TYPE_SUPERSECTION   = 0x40002;  // Gigantic pages

    constexpr uint32_t PML2_PAGE_TABLE_PXN      = 0x04;

    constexpr uint32_t PML2_SECTION_PXN         = 0x01;

    // Short-format second-level translation table (PML1)
    //    - Can contain small or large pages
    constexpr uint32_t PML1_PAGE_INVALID        = 0x0000;
    constexpr uint32_t PML1_PAGE_LARGE          = 0x0001;
    constexpr uint32_t PML1_PAGE_SMALL          = 0x0002;

    constexpr uint32_t PML1_PAGE_XN             = 0x0001;   // Execute Never, only for PML1_PAGE_SMALL
    constexpr uint32_t PML1_PAGE_B              = 0x0004;
    constexpr uint32_t PML1_PAGE_C              = 0x0008;
    constexpr uint32_t PML1_PAGE_AP0            = 0x0010;
    constexpr uint32_t PML1_PAGE_AP1            = 0x0020;
    constexpr uint32_t PML1_PAGE_TEX0           = 0x0040;
    constexpr uint32_t PML1_PAGE_TEX1           = 0x0080;
    constexpr uint32_t PML1_PAGE_TEX2           = 0x0100;
    constexpr uint32_t PML1_PAGE_AP2            = 0x0200;
    constexpr uint32_t PML1_PAGE_SHAREABLE      = 0x0400;
    constexpr uint32_t PML1_PAGE_NOT_GLOBAL     = 0x0800;

    constexpr uint32_t PML1_PAGE_NO_ACCESS      = 0;
    constexpr uint32_t PML1_PAGE_RW             = PML1_PAGE_AP0;
    constexpr uint32_t PML1_PAGE_RW_USER_RO     = PML1_PAGE_AP1;
    constexpr uint32_t PML1_PAGE_RW_USER_RW     = PML1_PAGE_AP1 | PML1_PAGE_AP0;
    constexpr uint32_t PML1_PAGE_RO             = PML1_PAGE_AP2;
    constexpr uint32_t PML1_PAGE_RO_USER_RO     = PML1_PAGE_AP2 | PML1_PAGE_AP1 | PML1_PAGE_AP0;
}


namespace LongDescriptor
{
    using Entry = uint64_t;
    using physaddr_t = uint64_t;

    // Normal pages are 4 KB
    constexpr auto MEMORY_PAGE_SHIFT = 12;
    constexpr auto MEMORY_PAGE_SIZE = 4096;

    // Large pages ("blocks") are 2 MB
    constexpr auto MEMORY_LARGE_PAGE_SHIFT = 21;
    constexpr auto MEMORY_LARGE_PAGE_SIZE =  2 * 1024 * 1024;

    // Huge pages ("blocks") are 1 GB
    constexpr auto MEMORY_HUGE_PAGE_SHIFT = 30;
    constexpr auto MEMORY_HUGE_PAGE_SIZE = 1024 * 1024 * 1024;
}
*/


/*
    For now, assume Short-descriptor format
*/

typedef uint32_t physaddr_t;

// Normal pages are 4 KB
constexpr auto MEMORY_PAGE_SHIFT = 12;
constexpr auto MEMORY_PAGE_SIZE = 4096;

// Large pages ("sections") are 1 MB
constexpr auto MEMORY_LARGE_PAGE_SHIFT = 20;
constexpr auto MEMORY_LARGE_PAGE_SIZE = 1024 * 1024;


namespace arm
{
    constexpr uint32_t PAGE_XN             = 1 << 0;    // Execute-never
    constexpr uint32_t PAGE_SMALL          = 1 << 1;    // Descriptor type = 4K page
    constexpr uint32_t PAGE_B              = 1 << 2;    // Controls caching
    constexpr uint32_t PAGE_C              = 1 << 3;    // Controls caching
    constexpr uint32_t PAGE_AP0            = 1 << 4;    // Access flag
    constexpr uint32_t PAGE_AP1            = 1 << 5;    // Accessible to user space
    constexpr uint32_t PAGE_TEX0           = 1 << 6;    // Controls caching
    constexpr uint32_t PAGE_TEX1           = 1 << 7;    // Controls caching
    constexpr uint32_t PAGE_TEX2           = 1 << 8;    // Controls caching
    constexpr uint32_t PAGE_AP2            = 1 << 9;    // Read-only
    constexpr uint32_t PAGE_SHAREABLE      = 1 << 10;
    constexpr uint32_t PAGE_NOT_GLOBAL     = 1 << 11;

    // Aliases
    constexpr auto PAGE_AF       = PAGE_AP0;    // Accessed Flag
    constexpr auto PAGE_USER     = PAGE_AP1;    // Accessible to user space
    constexpr auto PAGE_READONLY = PAGE_AP2;    // Read-only
} // namespace arm


constexpr inline physaddr_t GetPageFlags(PageType type)
{
    switch (type)
    {
        case PageType::KernelCode:          return arm::PAGE_SMALL |                                 arm::PAGE_READONLY;
        case PageType::KernelData_RO:       return arm::PAGE_SMALL | arm::PAGE_XN |                  arm::PAGE_READONLY;
        case PageType::KernelData_RW:       return arm::PAGE_SMALL | arm::PAGE_XN;
        case PageType::UserCode:            return arm::PAGE_SMALL |                arm::PAGE_USER | arm::PAGE_READONLY;
        case PageType::UserData_RO:         return arm::PAGE_SMALL | arm::PAGE_XN | arm::PAGE_USER | arm::PAGE_READONLY;
        case PageType::UserData_RW:         return arm::PAGE_SMALL | arm::PAGE_XN | arm::PAGE_USER;
        case PageType::MMIO:                return arm::PAGE_SMALL | arm::PAGE_XN; /* todo: disable caching */
        case PageType::VideoFramebuffer:    return arm::PAGE_SMALL | arm::PAGE_XN; /* todo: enable write-combining */
    }

    __builtin_unreachable();
}



#endif
