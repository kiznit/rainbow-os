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
#include <metal/arch.hpp>
#include <metal/helpers.hpp>
#include <metal/log.hpp>
#include <vector>

static_assert(mtl::kMemoryPageSize == efi::kPageSize);

namespace
{
    std::vector<efi::MemoryDescriptor> g_freeMemory;
}

void MemoryInitialize(const efi::MemoryDescriptor* descriptors, size_t descriptorCount)
{
    uint64_t usablePages{};
    uint64_t usedPages{};
    uint64_t freePages{};
    uint64_t reservedPages{};

    g_freeMemory.reserve(descriptorCount);

    for (size_t i = 0; i != descriptorCount; ++i)
    {
        const auto& descriptor = descriptors[i];

        switch (descriptor.type)
        {
        case efi::MemoryType::Conventional:
            if (descriptor.attributes & efi::MemoryAttribute::WriteBack)
            {
                freePages += descriptor.numberOfPages;
                g_freeMemory.emplace_back(descriptor);
            }
            else
                reservedPages += descriptor.numberOfPages;
            usablePages += descriptor.numberOfPages;
            break;

        case efi::MemoryType::LoaderCode:
        case efi::MemoryType::LoaderData:
        case efi::MemoryType::BootServicesCode:
        case efi::MemoryType::BootServicesData:
        case efi::MemoryType::AcpiReclaimable:
            if (descriptor.attributes & efi::MemoryAttribute::WriteBack)
                usedPages += descriptor.numberOfPages;
            else
                reservedPages += descriptor.numberOfPages;
            usablePages += descriptor.numberOfPages;
            break;

        case efi::MemoryType::Reserved:
        case efi::MemoryType::RuntimeServicesCode:
        case efi::MemoryType::RuntimeServicesData:
        case efi::MemoryType::Unusable:
        case efi::MemoryType::AcpiNonVolatile:
            reservedPages += descriptor.numberOfPages;
            break;

        default:
            break;
        }
    }

    MTL_LOG(Info) << "[KRNL] Usable memory  : " << mtl::hex(usablePages * mtl::kMemoryPageSize);
    MTL_LOG(Info) << "[KRNL] Used memory    : " << mtl::hex(usedPages * mtl::kMemoryPageSize);
    MTL_LOG(Info) << "[KRNL] Free memory    : " << mtl::hex(freePages * mtl::kMemoryPageSize);
    MTL_LOG(Info) << "[KRNL] Reserved memory: " << mtl::hex(reservedPages * mtl::kMemoryPageSize);
}

// TODO: support for non-contiguous frames
std::expected<PhysicalAddress, ErrorCode> AllocFrames(size_t count)
{
    efi::MemoryDescriptor* candidate{};

    for (auto& descriptor : g_freeMemory)
    {
        assert(descriptor.type == efi::MemoryType::Conventional);
        assert(descriptor.attributes & efi::MemoryAttribute::WriteBack);

        if (descriptor.numberOfPages < count)
            continue;

        if (!candidate || descriptor.physicalStart > candidate->physicalStart)
            candidate = &descriptor;
    }

    if (!candidate)
        return std::unexpected(ErrorCode::ENOMEM);

    const auto frames = candidate->physicalStart + (candidate->numberOfPages - count) * mtl::kMemoryPageSize;
    candidate->numberOfPages -= count;

    if (candidate->numberOfPages == 0)
    {
        std::swap(*candidate, g_freeMemory.back());
        g_freeMemory.pop_back();
    }

    // TODO: track the newly allocated memory

    return frames;
}

std::expected<void, ErrorCode> FreeFrames(PhysicalAddress frames, size_t count)
{
    // TODO
    (void)frames;
    (void)count;

    return {};
}

std::expected<void, ErrorCode> VirtualAlloc(void* address, size_t size)
{
    // TODO: need some validation that [address, address + size] is not already mapped
    size = mtl::AlignUp(size, mtl::kMemoryPageSize);

    const auto pageCount = size >> mtl::kMemoryPageShift;

    // TODO: support for non-contiguous frames
    const auto frames = AllocFrames(pageCount);
    if (!frames)
        return std::unexpected(frames.error());

    auto result = MapPages(frames.value(), address, pageCount, mtl::PageFlags::KernelData_RW);
    if (!result)
        return std::unexpected(result.error());

    return {};
}

std::expected<void, ErrorCode> VirtualFree(void* address, size_t size)
{
    // TODO
    (void)address;
    (void)size;

    return {};
}