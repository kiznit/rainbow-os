/*
    Copyright (c) 2024, Thierry Tremblay
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

#include "PageTable.hpp"
#include "boot.hpp"
#include <cassert>
#include <cstring>
#include <metal/exception.hpp>
#include <metal/helpers.hpp>
#include <metal/log.hpp>

PageTable::PageTable()
{
    auto root = AllocateZeroedPages(1, efi::MemoryType::KernelData);
    pml4 = reinterpret_cast<uint64_t*>(root);

    // Setup recursive mapping
    //      0xFFFFFF00 00000000 - 0xFFFFFF7F FFFFFFFF   Page Mapping Level 1 (Translation Table Level 3)
    //      0xFFFFFF7F 80000000 - 0xFFFFFF7F BFFFFFFF   Page Mapping Level 2 (Translation Table Level 2)
    //      0xFFFFFF7F BFC00000 - 0xFFFFFF7F BFDFFFFF   Page Mapping Level 3 (Translation Table Level 1)
    //      0xFFFFFF7F BFDFE000 - 0xFFFFFF7F BFDFEFFF   Page Mapping Level 4 (Translation Table Level 0)

    // We use entry 510 because the kernel occupies entry 511
    pml4[510] =
        (uintptr_t)pml4 | mtl::PageFlags::Valid | mtl::PageFlags::Table | mtl::PageFlags::WriteBack | mtl::PageFlags::AccessFlag;
}

void* PageTable::GetRaw() const
{
    auto descriptor =
        (uintptr_t)pml4 | mtl::PageFlags::Valid | mtl::PageFlags::Table | mtl::PageFlags::WriteBack | mtl::PageFlags::AccessFlag;
    return (void*)descriptor;
}

void PageTable::Map(mtl::PhysicalAddress physicalAddress, uintptr_t virtualAddress, size_t pageCount, mtl::PageFlags flags)
{
    assert(mtl::IsAligned(physicalAddress, mtl::kMemoryPageSize));
    assert(mtl::IsAligned(virtualAddress, mtl::kMemoryPageSize));

    while (pageCount-- > 0)
    {
        MapPage(physicalAddress, virtualAddress, flags);
        physicalAddress += mtl::kMemoryPageSize;
        virtualAddress += mtl::kMemoryPageSize;
    }
}

void PageTable::MapPage(mtl::PhysicalAddress physicalAddress, uintptr_t virtualAddress, mtl::PageFlags flags)
{
    assert(mtl::IsAligned(physicalAddress, mtl::kMemoryPageSize));
    assert(mtl::IsAligned(virtualAddress, mtl::kMemoryPageSize));

    // We should only be mapping pages to high address space.
    assert(virtualAddress >= 0xFFFF000000000000ull);

    const int i4 = (virtualAddress >> 39) & 0x1FF;
    const int i3 = (virtualAddress >> 30) & 0x1FF;
    const int i2 = (virtualAddress >> 21) & 0x1FF;
    const int i1 = (virtualAddress >> 12) & 0x1FF;

    if (!(pml4[i4] & mtl::PageFlags::Valid))
    {
        const auto page = AllocateZeroedPages(1, efi::MemoryType::KernelData);
        pml4[i4] = page | mtl::PageFlags::PageTable;
    }

    uint64_t* pml3 = (uint64_t*)(pml4[i4] & mtl::AddressMask);
    if (!(pml3[i3] & mtl::PageFlags::Valid))
    {
        const auto page = AllocateZeroedPages(1, efi::MemoryType::KernelData);
        pml3[i3] = page | mtl::PageFlags::PageTable;
    }

    uint64_t* pml2 = (uint64_t*)(pml3[i3] & mtl::AddressMask);
    if (!(pml2[i2] & mtl::PageFlags::Valid))
    {
        const auto page = AllocateZeroedPages(1, efi::MemoryType::KernelData);
        pml2[i2] = page | mtl::PageFlags::PageTable;
    }

    uint64_t* pml1 = (uint64_t*)(pml2[i2] & mtl::AddressMask);
    if (pml1[i1] & mtl::PageFlags::Valid)
    {
        MTL_LOG(Fatal) << "PageTable::MapPage() - There is already something there! (i1 = " << i1
                       << ", entry = " << mtl::hex(pml1[i1]) << ")";
        std::abort();
    }

    pml1[i1] = physicalAddress | flags;
}
