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

#include "MemoryMap.hpp"
#include "boot.hpp"
#include <algorithm>
#include <cassert>
#include <metal/log.hpp>

MemoryMap::MemoryMap(std::vector<efi::MemoryDescriptor> descriptors) : m_descriptors(std::move(descriptors))
{
}

std::expected<PhysicalAddress, bool> MemoryMap::AllocatePages(size_t numberOfPages)
{
    assert(numberOfPages > 0);

    // Allocate from highest available memory (low addresses are precious, on PC anyways).
    efi::MemoryDescriptor* candidate{};

    for (auto& descriptor : m_descriptors)
    {
        if (descriptor.type != efi::MemoryType::Conventional)
            continue;

        if (!(descriptor.attributes & efi::MemoryAttribute::WriteBack))
            continue;

        if (descriptor.numberOfPages < numberOfPages)
            continue;

        if (descriptor.physicalStart + numberOfPages * mtl::kMemoryPageSize > kMaxAllocationAddress)
            continue;

        if (!candidate || descriptor.physicalStart > candidate->physicalStart)
            candidate = &descriptor;
    }

    if (!candidate)
        return std::unexpected(false);

    if (candidate->numberOfPages == numberOfPages)
    {
        candidate->type = efi::MemoryType::LoaderData;
        return candidate->physicalStart;
    }

    // We have to be careful about recursion here, which can happen when growing the vector of memory descriptors.
    // Consider what would happen if:
    //      1) 'candidate' is the only block of available memory and
    //      2) the vector needs to grow

    const PhysicalAddress address = candidate->physicalStart + (candidate->numberOfPages - numberOfPages) * mtl::kMemoryPageSize;
    candidate->numberOfPages -= numberOfPages;

    // Track the newly allocated memory
    for (auto& descriptor : m_descriptors)
    {
        if (descriptor.type != efi::MemoryType::LoaderData || descriptor.attributes != candidate->attributes)
            continue;

        // Is the entry adjacent?
        if (address == descriptor.physicalStart + descriptor.numberOfPages * mtl::kMemoryPageSize)
        {
            descriptor.numberOfPages += numberOfPages;
            return address;
        }

        if (address + numberOfPages * mtl::kMemoryPageSize == descriptor.physicalStart)
        {
            descriptor.physicalStart = address;
            descriptor.numberOfPages += numberOfPages;
            return address;
        }
    }

    // We must create a new descriptor
    m_descriptors.emplace_back(efi::MemoryDescriptor{.type = efi::MemoryType::LoaderData,
                                                     .padding = 0,
                                                     .physicalStart = address,
                                                     .virtualStart = 0,
                                                     .numberOfPages = numberOfPages,
                                                     .attributes = candidate->attributes});

    return address;
}

void MemoryMap::Print() const
{
    MTL_LOG(Info) << "Memory map:";

    for (const auto& descriptor : m_descriptors)
    {
        MTL_LOG(Info) << "    " << mtl::hex(descriptor.physicalStart) << " - "
                      << mtl::hex(descriptor.physicalStart + descriptor.numberOfPages * mtl::kMemoryPageSize - 1) << ": "
                      << " " << efi::ToString(descriptor.type);
    }
}
