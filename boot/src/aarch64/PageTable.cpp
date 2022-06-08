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

#include "PageTable.hpp"
#include "../uefi.hpp"
#include <cassert>
#include <cstring>
#include <metal/exception.hpp>
#include <metal/helpers.hpp>
#include <metal/log.hpp>

PageTable::PageTable()
{
    MTL_LOG(Info) << "TTBR0_EL1: " << mtl::hex(mtl::aarch64_read_TTBR0_EL1());
    MTL_LOG(Info) << "TTBR1_EL1: " << mtl::hex(mtl::aarch64_read_TTBR1_EL1());
    MTL_LOG(Info) << "TCR_EL1  : " << mtl::hex(mtl::aarch64_read_TCR_EL1());

    auto root = AllocatePages(1);

    pml4 = reinterpret_cast<uint64_t*>(root);

    // TODO:
    // Setup recursive mapping
    //      0xFFFFFF00 00000000 - 0xFFFFFF7F FFFFFFFF   Page Mapping Level 1 PML1
    //      0xFFFFFF7F 80000000 - 0xFFFFFF7F BFFFFFFF   Page Mapping Level 2 PML2
    //      0xFFFFFF7F BFC00000 - 0xFFFFFF7F BFDFFFFF   Page Mapping Level 3 PML3

    // We use entry 510 because the kernel occupies entry 511
    // pml4[510] = (uintptr_t)pml4 | mtl::PageFlags::Write | mtl::PageFlags::Present;
}

void* PageTable::GetRaw() const
{
    // Return a level 0 descriptor
    auto descriptor =
        reinterpret_cast<uint64_t>(pml4) | mtl::PageFlags::Table | mtl::PageFlags::NSTable; // TODO: do we want the NSTable bit?

    // TODO: We will want to set UXNTable and APTable_Mask for kernel space in other levels

    return reinterpret_cast<void*>(descriptor);
}

mtl::PhysicalAddress PageTable::AllocatePages(size_t pageCount)
{
    if (auto p = ::AllocatePages(pageCount))
    {
        const auto page = *p;
        memset(reinterpret_cast<void*>(page), 0, pageCount * mtl::MemoryPageSize);
        return page;
    }

    MTL_LOG(Fatal) << "PageTable::AllocatePages() - Out of memory";
    std::abort();
}

void PageTable::Map(mtl::PhysicalAddress physicalAddress, uintptr_t virtualAddress, size_t pageCount, mtl::PageFlags flags)
{
    assert(mtl::is_aligned(physicalAddress, mtl::MemoryPageSize));
    assert(mtl::is_aligned(virtualAddress, mtl::MemoryPageSize));

    while (pageCount-- > 0)
    {
        MapPage(physicalAddress, virtualAddress, flags);
        physicalAddress += mtl::MemoryPageSize;
        virtualAddress += mtl::MemoryPageSize;
    }
}

void PageTable::MapPage(mtl::PhysicalAddress physicalAddress, uintptr_t virtualAddress, mtl::PageFlags flags)
{
    assert(mtl::is_aligned(physicalAddress, mtl::MemoryPageSize));
    assert(mtl::is_aligned(virtualAddress, mtl::MemoryPageSize));

    (void)physicalAddress;
    (void)virtualAddress;
    (void)flags;

    // const long i3 = (virtualAddress >> 30) & 0x1FF;
    // const long i2 = (virtualAddress >> 21) & 0x1FF;
    // const long i1 = (virtualAddress >> 12) & 0x1FF;

    // const uint64_t kernelSpaceFlags = (i4 == 0x1ff) ? (uint64_t)mtl::PageFlags::Global : 0;

    // uint64_t* pml3 = (uint64_t*)(pml4[i4] & mtl::AddressMask);
    // if (!(pml3[i3] & mtl::PageFlags::Present))
    // {
    //     const auto page = AllocatePages(1);
    //     pml3[i3] = page | mtl::PageFlags::Write | mtl::PageFlags::Present | kernelSpaceFlags;
    // }

    // uint64_t* pml2 = (uint64_t*)(pml3[i3] & mtl::AddressMask);
    // if (!(pml2[i2] & mtl::PageFlags::Present))
    // {
    //     const auto page = AllocatePages(1);
    //     pml2[i2] = page | mtl::PageFlags::Write | mtl::PageFlags::Present | kernelSpaceFlags;
    // }

    // uint64_t* pml1 = (uint64_t*)(pml2[i2] & mtl::AddressMask);
    // if (pml1[i1] & mtl::PageFlags::Present)
    // {
    //     MTL_LOG(Fatal) << "PageTable::MapPage() - There is already something there! (i1 = " << i1
    //                    << ", entry = " << mtl::hex(pml1[i1]) << ")";
    //     std::abort();
    // }

    // pml1[i1] = physicalAddress | flags | kernelSpaceFlags;
}
