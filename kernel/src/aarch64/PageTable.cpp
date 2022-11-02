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

#include "memory.hpp"

// Where we can find the page tables in virtual memory
static uint64_t* const vmm_pml4 = (uint64_t*)0xFFFFFF7FBFDFE000ull;
static uint64_t* const vmm_pml3 = (uint64_t*)0xFFFFFF7FBFC00000ull;
static uint64_t* const vmm_pml2 = (uint64_t*)0xFFFFFF7F80000000ull;
static uint64_t* const vmm_pml1 = (uint64_t*)0xFFFFFF0000000000ull;

static void InvalidatePage(const void* address)
{
    // Found this sequence in the ARMv8 manual, might be overkill
    // https://developer.arm.com/documentation/101811/0102/Translation-Lookaside-Buffer-maintenance
    mtl::aarch64_dsb_ishst();
    mtl::aarch64_tlbi_vae1(address);
    mtl::aarch64_dsb_ish();
    mtl::aarch64_isb();
}

std::expected<void, ErrorCode> MapPages(efi::PhysicalAddress physicalAddress, const void* virtualAddress, int pageCount,
                                        mtl::PageFlags pageFlags)
{
    // MTL_LOG(Debug) << "MapPages: " << mtl::hex(physicalAddress) << ", " << virtualAddress << ", " << pageCount << ", "
    //                << mtl::hex(pageFlags);

    assert(mtl::IsAligned(physicalAddress, mtl::kMemoryPageSize));
    assert(mtl::IsAligned(virtualAddress, mtl::kMemoryPageSize));

    // TODO: need critical sections here

    // On aarch64, we can only map pages in high address space. I don't believe we have a need to map anything in low address space.
    // We assert to make sure we don't get any surprises.
    assert((uintptr_t)virtualAddress >= 0xFFFF000000000000ull);

    for (auto page = 0; page != pageCount; ++page)
    {
        const auto addr = (uint64_t)virtualAddress;
        const auto i4 = (addr >> 39) & 0x1FF;
        const auto i3 = (addr >> 30) & 0x3FFFF;
        const auto i2 = (addr >> 21) & 0x7FFFFFF;
        const auto i1 = (addr >> 12) & 0xFFFFFFFFFul;

        if (!(vmm_pml4[i4] & mtl::PageFlags::Valid))
        {
            if (const auto frame = AllocFrames(1))
            {
                vmm_pml4[i4] = *frame | mtl::PageFlags::Valid | mtl::PageFlags::Table | mtl::PageFlags::AccessFlag;
                volatile auto p = (char*)vmm_pml3 + (i4 << 12);
                InvalidatePage(p);

                memset(p, 0, mtl::kMemoryPageSize);
            }
            else
            {
                return std::unexpected(ErrorCode::OutOfMemory);
            }
        }

        if (!(vmm_pml3[i3] & mtl::PageFlags::Valid))
        {
            if (const auto frame = AllocFrames(1))
            {
                vmm_pml3[i3] = *frame | mtl::PageFlags::Valid | mtl::PageFlags::Table | mtl::PageFlags::AccessFlag;
                volatile auto p = (char*)vmm_pml2 + (i3 << 12);
                InvalidatePage(p);

                memset(p, 0, mtl::kMemoryPageSize);
            }
            else
            {
                return std::unexpected(ErrorCode::OutOfMemory);
            }
        }

        if (!(vmm_pml2[i2] & mtl::PageFlags::Valid))
        {
            if (const auto frame = AllocFrames(1))
            {
                vmm_pml2[i2] = *frame | mtl::PageFlags::Valid | mtl::PageFlags::Table | mtl::PageFlags::AccessFlag;
                volatile auto p = (char*)vmm_pml1 + (i2 << 12);
                InvalidatePage(p);

                memset(p, 0, mtl::kMemoryPageSize);
            }
            else
            {
                return std::unexpected(ErrorCode::OutOfMemory);
            }
        }

        if (vmm_pml1[i1] & mtl::PageFlags::Valid) [[unlikely]]
        {
            if (!((vmm_pml1[i1] & mtl::PageFlags::FlagsMask) == pageFlags))
            {
                MTL_LOG(Fatal) << "Failed to map " << mtl::hex(physicalAddress) << " to " << virtualAddress;
                MTL_LOG(Fatal) << "Previous entry: " << mtl::hex(vmm_pml1[i1])
                               << ", new one: " << mtl::hex(physicalAddress | pageFlags);
                assert(0 && "There is already a page mapped at this address");
            }
        }
        else
        {
            vmm_pml1[i1] = physicalAddress | pageFlags;
            InvalidatePage(virtualAddress);
        }

        // Next page...
        physicalAddress += mtl::kMemoryPageSize;
        virtualAddress = mtl::AdvancePointer(virtualAddress, mtl::kMemoryPageSize);
    }

    return {};
}

std::expected<void, ErrorCode> UnmapPages(const void* virtualAddress, int pageCount)
{
    (void)virtualAddress;
    (void)pageCount;

    // TODO

    return {};
}
