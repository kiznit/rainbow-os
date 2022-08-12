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
#include <metal/log.hpp>
#include <vector>

static std::vector<efi::MemoryDescriptor> s_freeMemory;

void memory_initialize(const efi::MemoryDescriptor* descriptors, size_t descriptorCount)
{
    uint64_t usablePages{};
    uint64_t usedPages{};
    uint64_t freePages{};
    uint64_t reservedPages{};

    s_freeMemory.reserve(descriptorCount);

    for (size_t i = 0; i != descriptorCount; ++i)
    {
        const auto& descriptor = descriptors[i];

        switch (descriptor.type)
        {
        case efi::MemoryType::Conventional:
            if (descriptor.attributes & efi::MemoryAttribute::WriteBack)
            {
                freePages += descriptor.numberOfPages;
                s_freeMemory.emplace_back(descriptor);
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

    MTL_LOG(Info) << "Usable memory  : " << mtl::hex(usablePages * mtl::MemoryPageSize);
    MTL_LOG(Info) << "Used memory    : " << mtl::hex(usedPages * mtl::MemoryPageSize);
    MTL_LOG(Info) << "Free memory    : " << mtl::hex(freePages * mtl::MemoryPageSize);
    MTL_LOG(Info) << "Reserved memory: " << mtl::hex(reservedPages * mtl::MemoryPageSize);
}

std::expected<PhysicalAddress, ErrorCode> alloc_frames(size_t count)
{
    efi::MemoryDescriptor* candidate{};

    for (auto& descriptor : s_freeMemory)
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

    const auto frames = candidate->physicalStart + (candidate->numberOfPages - count) * mtl::MemoryPageSize;
    candidate->numberOfPages -= count;

    if (candidate->numberOfPages == 0)
    {
        std::swap(*candidate, s_freeMemory.back());
        s_freeMemory.pop_back();
    }

    // TODO: track the newly allocated memory

    return frames;
}

void free_frames(PhysicalAddress frames, size_t count)
{
    // TODO
    (void)frames;
    (void)count;
}
