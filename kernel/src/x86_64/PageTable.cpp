/*
    Copyright (c) 2023, Thierry Tremblay
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

#include "memory.hpp"
#include <cassert>
#include <metal/helpers.hpp>
#include <metal/log.hpp>

// Where we can find the page tables in virtual memory
//
// How to calculate these magic numbers?
//
// 64 bits virtual address on x86_64 (4 levels page table):
//
// 12 bits (bits 0-11) represents the byte offset in a page (4096 bytes)
// 9 bits per translation level (4 x 9 = 36 bits)
// The top 16 bits have to be a copy of bit #47 to form a canonical address
// Total: 12 + 4 * 9 + 16 = 64 bits
//
// Each translation level is 9 bits. The easiest is to use octal to represent them (o000 - o777).
// Now if you want to use slot 510 for the recursive mapping, you convert the slot number to octal (o776) in this case.
// PML4 is at o776 776 776 776, converted to hexadecimal yields 0xFF7FBFDFE.
// Add the page offset and you get 0xFF7FBFDFE000
// Finally pad the top bits with a copy of bit 47 and you get and you get 0xFFFFFF7FBFDFE000
//
// Similarly, you can calculate pml3, pml2 and pml1:
//      pml4: o776776776776 --> 0xFFFFFF7FBFDFE000
//      pml3: o776776776000 --> 0xFFFFFF7FBFC00000
//      pml2: o776776000000 --> 0xFFFFFF7F80000000
//      pml1: o776000000000 --> 0xFFFFFF0000000000
// The end of the recursive page tables can be found similarly:
//      Top : o776777777777 --> 0xFFFFFF7FFFFFFFFF

static uint64_t* const vmm_pml4 = (uint64_t*)0xFFFFFF7FBFDFE000ull;
static uint64_t* const vmm_pml3 = (uint64_t*)0xFFFFFF7FBFC00000ull;
static uint64_t* const vmm_pml2 = (uint64_t*)0xFFFFFF7F80000000ull;
static uint64_t* const vmm_pml1 = (uint64_t*)0xFFFFFF0000000000ull;

mtl::expected<void, ErrorCode> MapPages(efi::PhysicalAddress physicalAddress, const void* virtualAddress, int pageCount,
                                        mtl::PageFlags pageFlags)
{
    // MTL_LOG(Debug) << "MapPages: " << mtl::hex(physicalAddress) << ", " << virtualAddress << ", " << pageCount << ", "
    //                << mtl::hex(pageFlags);

    assert(mtl::IsAligned(physicalAddress, mtl::kMemoryPageSize));
    assert(mtl::IsAligned(virtualAddress, mtl::kMemoryPageSize));

    // TODO: need critical sections here

    for (auto page = 0; page != pageCount; ++page)
    {
        const auto addr = (uint64_t)virtualAddress;
        const auto i4 = (addr >> 39) & 0x1FF;
        const auto i3 = (addr >> 30) & 0x3FFFF;
        const auto i2 = (addr >> 21) & 0x7FFFFFF;
        const auto i1 = (addr >> 12) & 0xFFFFFFFFFul;

        const uint64_t kernelSpaceFlags = (i4 == 0x1ff) ? mtl::PageFlags::Global : 0;

        if (!(vmm_pml4[i4] & mtl::PageFlags::Present))
        {
            if (const auto frame = AllocFrames(1))
            {
                vmm_pml4[i4] = *frame | mtl::PageFlags::PageTable | kernelSpaceFlags | (pageFlags & mtl::PageFlags::User);
                volatile auto p = (char*)vmm_pml3 + (i4 << 12);
                mtl::x86_invlpg(p);

                memset(p, 0, mtl::kMemoryPageSize);
            }
            else
            {
                return mtl::unexpected(ErrorCode::OutOfMemory);
            }
        }

        if (!(vmm_pml3[i3] & mtl::PageFlags::Present))
        {
            if (const auto frame = AllocFrames(1))
            {
                vmm_pml3[i3] = *frame | mtl::PageFlags::PageTable | kernelSpaceFlags | (pageFlags & mtl::PageFlags::User);
                volatile auto p = (char*)vmm_pml2 + (i3 << 12);
                mtl::x86_invlpg(p);

                memset(p, 0, mtl::kMemoryPageSize);
            }
            else
            {
                return mtl::unexpected(ErrorCode::OutOfMemory);
            }
        }

        if (!(vmm_pml2[i2] & mtl::PageFlags::Present))
        {
            if (const auto frame = AllocFrames(1))
            {
                vmm_pml2[i2] = *frame | mtl::PageFlags::PageTable | kernelSpaceFlags | (pageFlags & mtl::PageFlags::User);
                volatile auto p = (char*)vmm_pml1 + (i2 << 12);
                mtl::x86_invlpg(p);

                memset(p, 0, mtl::kMemoryPageSize);
            }
            else
            {
                return mtl::unexpected(ErrorCode::OutOfMemory);
            }
        }

        if (vmm_pml1[i1] & mtl::PageFlags::Present) [[unlikely]]
        {
            if (!((vmm_pml1[i1] & mtl::PageFlags::FlagsMask) == (pageFlags | kernelSpaceFlags)))
            {
                MTL_LOG(Fatal) << "Failed to map " << mtl::hex(physicalAddress) << " to " << virtualAddress;
                MTL_LOG(Fatal) << "Previous entry: " << mtl::hex(vmm_pml1[i1])
                               << ", new one: " << mtl::hex(physicalAddress | pageFlags | kernelSpaceFlags);
                assert(0 && "There is already a page mapped at this address");
            }
        }
        else
        {
            vmm_pml1[i1] = physicalAddress | pageFlags | kernelSpaceFlags;
            mtl::x86_invlpg(virtualAddress);
        }

        // Next page...
        physicalAddress += mtl::kMemoryPageSize;
        virtualAddress = mtl::AdvancePointer(virtualAddress, mtl::kMemoryPageSize);
    }

    return {};
}

mtl::expected<void, ErrorCode> UnmapPages(const void* virtualAddress, int pageCount)
{
    assert(mtl::IsAligned(virtualAddress, mtl::kMemoryPageSize));
    // TODO: validate that the memory we are trying to free is part of the heap!

    // TODO: need critical section here...
    // TODO: need to update memory map region and track holes
    // TODO: check if we can free page tables (pml1, pml2, pml3)
    const auto addr = (uint64_t)virtualAddress;
    auto i1 = (addr >> 12) & 0xFFFFFFFFFul;

    for (; pageCount > 0; --pageCount, ++i1)
    {
        if (vmm_pml1[i1] & mtl::PageFlags::Present) // TODO: should be an assert?
        {
            // TODO: free multiple frames at once if possible
            FreeFrames(vmm_pml1[i1] & mtl::PageFlags::AddressMask, 1);
            vmm_pml1[i1] = 0;
            // TODO: TLB shutdown (SMP)
            mtl::x86_invlpg(virtualAddress);
        }
    }

    return {};
}
