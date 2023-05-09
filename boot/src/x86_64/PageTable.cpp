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
#include "boot.hpp"
#include <cassert>
#include <cstring>
#include <metal/exception.hpp>
#include <metal/helpers.hpp>
#include <metal/log.hpp>

PageTable::PageTable()
{
    // To keep things simple, we are going to identity-map the first 4 GB of memory.
    // The kernel will be mapped outside of the first 4 GB of memory.
    pml4 = (uint64_t*)AllocateZeroedPages(1, efi::MemoryType::KernelData);
    auto pml3 = (uint64_t*)AllocateZeroedPages(5, efi::MemoryType::LoaderData);
    auto pml2 = mtl::AdvancePointer(pml3, mtl::kMemoryPageSize);

    const auto identityPageFlags = mtl::PageFlags::Present | mtl::PageFlags::Write | mtl::PageFlags::WriteBack;

    // 1 entry = 512 GB
    pml4[0] = (uintptr_t)pml3 | identityPageFlags;

    // 4 entries = 4 x 1GB = 4 GB
    pml3[0] = (uintptr_t)&pml2[0] | identityPageFlags;
    pml3[1] = (uintptr_t)&pml2[512] | identityPageFlags;
    pml3[2] = (uintptr_t)&pml2[1024] | identityPageFlags;
    pml3[3] = (uintptr_t)&pml2[1536] | identityPageFlags;

    // 2048 entries = 2048 * 2 MB = 4 GB
    for (uint64_t i = 0; i != 2048; ++i)
    {
        pml2[i] = i * 512 * mtl::kMemoryPageSize | identityPageFlags | mtl::PageFlags::Size;
    }

    // Setup recursive mapping
    //      0xFFFFFF00 00000000 - 0xFFFFFF7F FFFFFFFF   Page Mapping Level 1 (Page Tables)
    //      0xFFFFFF7F 80000000 - 0xFFFFFF7F BFFFFFFF   Page Mapping Level 2 (Page Directories)
    //      0xFFFFFF7F BFC00000 - 0xFFFFFF7F BFDFFFFF   Page Mapping Level 3 (PDPTs / Page-Directory-Pointer Tables)
    //      0xFFFFFF7F BFDFE000 - 0xFFFFFF7F BFDFEFFF   Page Mapping Level 4 (PML4)

    // We use entry 510 because the kernel occupies entry 511
    pml4[510] = (uintptr_t)pml4 | mtl::PageFlags::Present | mtl::PageFlags::NX | mtl::PageFlags::Write | mtl::PageFlags::Global;
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

    if (!(pml4[i4] & mtl::PageFlags::Present))
    {
        const auto page = AllocateZeroedPages(1, efi::MemoryType::KernelData);
        pml4[i4] = page | mtl::PageFlags::PageTable | mtl::PageFlags::Global;
    }

    uint64_t* pml3 = (uint64_t*)(pml4[i4] & mtl::AddressMask);
    if (!(pml3[i3] & mtl::PageFlags::Present))
    {
        const auto page = AllocateZeroedPages(1, efi::MemoryType::KernelData);
        pml3[i3] = page | mtl::PageFlags::PageTable | mtl::PageFlags::Global;
    }

    uint64_t* pml2 = (uint64_t*)(pml3[i3] & mtl::AddressMask);
    if (!(pml2[i2] & mtl::PageFlags::Present))
    {
        const auto page = AllocateZeroedPages(1, efi::MemoryType::KernelData);
        pml2[i2] = page | mtl::PageFlags::PageTable | mtl::PageFlags::Global;
    }

    uint64_t* pml1 = (uint64_t*)(pml2[i2] & mtl::AddressMask);
    if (pml1[i1] & mtl::PageFlags::Present)
    {
        MTL_LOG(Fatal) << "PageTable::MapPage() - There is already something there! (i1 = " << i1
                       << ", entry = " << mtl::hex(pml1[i1]) << ")";
        std::abort();
    }

    pml1[i1] = physicalAddress | flags | mtl::PageFlags::Global;
}