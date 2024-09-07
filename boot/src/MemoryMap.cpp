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

#include "MemoryMap.hpp"
#include "boot.hpp"
#include <algorithm>
#include <cassert>
#include <metal/log.hpp>

MemoryMap::MemoryMap(mtl::vector<efi::MemoryDescriptor> descriptors, const mtl::vector<efi::MemoryDescriptor>& customMemoryTypes)
    : m_descriptors(std::move(descriptors))
{
    for (const auto& descriptor : customMemoryTypes)
        SetMemoryType(descriptor.physicalStart, descriptor.numberOfPages, descriptor.type);
}

mtl::optional<PhysicalAddress> MemoryMap::AllocatePages(size_t pageCount, efi::MemoryType memoryType)
{
    assert(pageCount > 0);

    // Allocate from highest available memory (low addresses are precious, on PC anyways).
    efi::MemoryDescriptor* candidate{};

    for (auto& descriptor : m_descriptors)
    {
        if (descriptor.type != efi::MemoryType::Conventional)
            continue;

        if (!(descriptor.attributes & efi::MemoryAttribute::WriteBack))
            continue;

        if (descriptor.numberOfPages < pageCount)
            continue;

        if (descriptor.physicalStart + pageCount * mtl::kMemoryPageSize > kMaxAllocationAddress)
            continue;

        if (!candidate || descriptor.physicalStart > candidate->physicalStart)
            candidate = &descriptor;
    }

    if (!candidate)
        return {};

    const PhysicalAddress address = candidate->physicalStart + (candidate->numberOfPages - pageCount) * mtl::kMemoryPageSize;

    SetMemoryType(address, pageCount, memoryType);

    return mtl::make_optional(address);
}

void MemoryMap::SetMemoryType(efi::PhysicalAddress address, size_t pageCount, efi::MemoryType memoryType)
{
    // This function assumes that there is already a descriptor for the specified memory range.
    // The existing descriptor might get split into up to three descriptors (so two extra ones).
    // We can't have allocations happening will we modify the descriptors, so we make sure to
    // reserve enough space beforehand.
    m_descriptors.reserve(m_descriptors.size() + 2);

    const auto start = address;
    const auto end = address + pageCount * mtl::kMemoryPageSize;

    // Now it is safe to find the descriptor we are interested in and split it as needed
    const auto descriptor = std::find_if(m_descriptors.begin(), m_descriptors.end(), [=](const efi::MemoryDescriptor& descriptor) {
        return start >= descriptor.physicalStart &&
               end <= descriptor.physicalStart + descriptor.numberOfPages * mtl::kMemoryPageSize;
    });
    assert(descriptor != m_descriptors.end());

    const auto descriptorStart = descriptor->physicalStart;
    const auto descriptorEnd = descriptor->physicalStart + descriptor->numberOfPages * mtl::kMemoryPageSize;

    assert(start >= descriptorStart);
    assert(end <= descriptorEnd);

    // Left bit
    if (descriptorStart < start)
    {
        m_descriptors.emplace_back(efi::MemoryDescriptor{.type = descriptor->type,
                                                         .padding = 0,
                                                         .physicalStart = descriptorStart,
                                                         .virtualStart = 0,
                                                         .numberOfPages = (start - descriptorStart) >> mtl::kMemoryPageShift,
                                                         .attributes = descriptor->attributes});
    }

    // Right bit
    if (descriptorEnd > end)
    {
        m_descriptors.emplace_back(efi::MemoryDescriptor{.type = descriptor->type,
                                                         .padding = 0,
                                                         .physicalStart = end,
                                                         .virtualStart = 0,
                                                         .numberOfPages = (descriptorEnd - end) >> mtl::kMemoryPageShift,
                                                         .attributes = descriptor->attributes});
    }

    // Middle bit
    descriptor->type = memoryType;
    descriptor->physicalStart = address;
    descriptor->numberOfPages = pageCount;
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
