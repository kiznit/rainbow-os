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

        Table with 4K

          Page Table Level      Bits        ARM Name
          ---------------------------------------------------------------------------------------------------
                  4            9 bits       Level 0 table (512 GB / entry)
                  3            9 bits       Level 1 table (1 GB / entry)
                  2            9 bits       Level 2 table (2 MB / entry)
                  1            9 bits       Level 3 table (4 KB / entry)
               (page)         12 bits       Page
          ---------------------------------------------------------------------------------------------------
                              48 bits       Virtual address size
                              48 bits       Physical address size
                               256 TB       Addressable Physical Memory

        Reference:
            https://medium.com/@om.nara/arm64-normal-memory-attributes-6086012fa0e3
            https://developer.arm.com/documentation/101811/0102/Address-spaces
            https://developer.arm.com/documentation/101811/0102/Controlling-address-translation-Translation-table-format
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

    enum PageFlags : uint64_t
    {
        Valid = 1 << 0,         // Descriptor is valid
        Table = 1 << 1,         // Entry is a page table
        Page = 1 << 1,          // Entry is a page (same as Table, ugh)
        MAIR = 7 << 2,          // Index into the MAIR_ELn (similar to x86 PATs)
        NS = 1 << 5,            // Security bit, but only at EL3 and Secure EL1
        AP1 = 1 << 6,           // EL0 (user) access (aka PAGE_USER on x86)
        AP2 = 1 << 7,           // Read only (opposite of PAGE_WRITE on x86)
        ShareableMask = 3 << 8, // Shareable
        AccessFlag = 1 << 10,   // Access flag (if 0, will trigger a page fault)

        // Memory Attribute Indirection Register (MAIR)
        // These happen to match what UEFI configures
        WriteBack = 0 << 2,      // MAIR index 0
        WriteThrough = 1 << 2,   // MAIR index 1
        Uncacheable = 2 << 2,    // MAIR index 2
        WriteCombining = 3 << 2, // MAIR index 3
        MAIR_4 = 4 << 2,         // MAIR index 4
        MAIR_5 = 5 << 2,         // MAIR index 5
        MAIR_6 = 6 << 2,         // MAIR index 6
        MAIR_7 = 7 << 2,         // MAIR index 7

        // Bits 12..47 are the address mask
        AddressMask = 0x0000FFFFFFFFF000ull,

        // Bits 51..48 are reserved

        DirtyBitModifier = 1ull << 51, // Dirty Bit Modifier
        Contiguous = 1ull << 52,       // Optimization to efficiently use TLB space
        PXN = 1ull << 53,              // Privileged eXecute Never
        UXN = 1ull << 54,              // Unprivileged eXecute Never

        // bits 55-58 are reserved for software use

        // https://medium.com/@om.nara/arm64-normal-memory-attributes-6086012fa0e3
        PXNTable = 1ull << 59,    // Privileged eXecute Never
        UXNTable = 1ull << 60,    // Unprivileged eXecute Never
        APTableMask = 3ull << 61, // Access Permission limits for subsequent levels of lookup
        NSTable = 1ull << 63,     // (0 - Secure PA space, 1 - Non-Secure)

        // Aliases
        User = AP1,     // Accessible to user space
        ReadOnly = AP2, // Read-only

        FlagsMask = ~AddressMask & ~DirtyBitModifier,

        // Page types
        KernelCode = Valid | Page | AccessFlag | UXN | ReadOnly | WriteBack,
        KernelData_RO = Valid | Page | AccessFlag | UXN | PXN | ReadOnly | WriteBack,
        KernelData_RW = Valid | Page | AccessFlag | UXN | PXN | WriteBack,
        UserCode = Valid | Page | AccessFlag | User | ReadOnly | WriteBack,
        UserData_RO = Valid | Page | AccessFlag | UXN | PXN | User | ReadOnly | WriteBack,
        UserData_RW = Valid | Page | AccessFlag | UXN | PXN | User | WriteBack,
        MMIO = Valid | Page | AccessFlag | UXN | PXN | Uncacheable,
        VideoFrameBuffer = Valid | Page | AccessFlag | UXN | PXN | WriteCombining,
    };

    // MAIR Memory Types
    enum Mair : uint64_t
    {
        MairUncacheable = 0x00,    // Device-nGnRnE (Device non-Gathering, non-Reordering, no Early Write Acknowledgement)
        MairWriteCombining = 0x44, // Normal Memory, Outer non-cacheable, Inner non-cacheable
        MairWriteThrough = 0xbb,   // Normal Memory, Outer Write-through non-transient, Inner Write-through non-transient
        MairWriteBack = 0xff,      // Normal Memory, Outer Write-back non-transient, Inner Write-back non-transient
    };

} // namespace mtl
