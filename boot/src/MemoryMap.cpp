/*
    Copyright (c) 2021, Thierry Tremblay
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
#include <algorithm>
#include <cassert>
#include <metal/log.hpp>

MemoryMap::MemoryMap(std::vector<efi::MemoryDescriptor>&& descriptors) : m_descriptors(std::move(descriptors))
{
    TidyUp();
}

std::expected<PhysicalAddress, bool> MemoryMap::AllocatePages(efi::MemoryType memoryType, size_t numberOfPages)
{
    assert(memoryType != efi::MemoryType::Conventional);
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

        if (!candidate || descriptor.physicalStart > candidate->physicalStart)
            candidate = &descriptor;
    }

    if (!candidate)
        return std::unexpected(false);

    if (candidate->numberOfPages == numberOfPages)
    {
        candidate->type = memoryType;
        return candidate->physicalStart;
    }
    else
    {
        // We have to be careful about recursion here. Calling SetMemoryRange() can grow the vector
        // of descriptors, which means that AllocatePages() could be called recursively. Consider
        // what would happen if:
        //      1) 'candidate' is the only block of 'Available' memory and
        //      2) the vector needs to grow

        const PhysicalAddress memory = candidate->physicalStart;

        candidate->physicalStart += numberOfPages * mtl::MemoryPageSize;
        candidate->numberOfPages -= numberOfPages;

        SetMemoryRange(memory, numberOfPages, memoryType, candidate->attributes);

        return memory;
    }
}

void MemoryMap::Print() const
{
    MTL_LOG(Info) << "Memory map:";

    for (const auto& descriptor : m_descriptors)
    {
        const char8_t* type = u8"Unknown";

        switch (descriptor.type)
        {
        case efi::MemoryType::Reserved:
            type = u8"Reserved";
            break;
        case efi::MemoryType::LoaderCode:
            type = u8"Bootloder Code";
            break;
        case efi::MemoryType::LoaderData:
            type = u8"Bootloader Data";
            break;
        case efi::MemoryType::BootServicesCode:
            type = u8"UEFI Boot Code";
            break;
        case efi::MemoryType::BootServicesData:
            type = u8"UEFI Boot Data";
            break;
        case efi::MemoryType::RuntimeServicesCode:
            type = u8"UEFI Runtime Code";
            break;
        case efi::MemoryType::RuntimeServicesData:
            type = u8"UEFI Runtime Data";
            break;
        case efi::MemoryType::Conventional:
            type = u8"Available";
            break;
        case efi::MemoryType::Unusable:
            type = u8"Unusable";
            break;
        case efi::MemoryType::AcpiReclaimable:
            type = u8"ACPI Reclaimable";
            break;
        case efi::MemoryType::AcpiNonVolatile:
            type = u8"ACPI Non-Volatile";
            break;
        case efi::MemoryType::MappedIo:
            type = u8"Mapped I/O";
            break;
        case efi::MemoryType::MappedIoPortSpace:
            type = u8"I/O Port Space";
            break;
        case efi::MemoryType::PalCode:
            type = u8"Processor Code";
            break;
        case efi::MemoryType::Persistent:
            type = u8"Persistent";
            break;
        case efi::MemoryType::Unaccepted:
            type = u8"Unaccepted";
            break;
        }

        MTL_LOG(Info) << "    " << mtl::hex(descriptor.physicalStart) << " - "
                      << mtl::hex(descriptor.physicalStart + descriptor.numberOfPages * mtl::MemoryPageSize - 1) << ": "
                      << mtl::hex(descriptor.attributes) << " " << type;
    }
}

void MemoryMap::SetMemoryRange(PhysicalAddress address, uint64_t numberOfPages, efi::MemoryType type,
                               efi::MemoryAttribute attributes)
{
    assert(mtl::is_aligned(address, mtl::MemoryPageSize));

    if (!numberOfPages)
        return;

    // We will do computations using page numbers instead of addresses. The main reason for this
    // is so that we don't have to deal with tricky overflow arithmetics when computing the end
    // address of a range at the end of memory space.
    const uint64_t startPage = address >> mtl::MemoryPageShift;
    const uint64_t endPage = startPage + numberOfPages;

    // Walk all existing entries looking for overlapping ranges
    for (auto& descriptor : m_descriptors)
    {
        const uint64_t otherStartPage = descriptor.physicalStart >> mtl::MemoryPageShift;
        const uint64_t otherEndPage = otherStartPage + descriptor.numberOfPages;

        // If range entirely before descriptor, continue
        if (endPage <= otherStartPage)
            continue;

        // If range entirely after descriptor, continue
        if (startPage >= otherEndPage)
            continue;

        // Copy the descriptor as we will overwrite it below
        const auto other = descriptor;

        // Handle overlap
        descriptor.type = std::max(type, other.type);
        descriptor.attributes = (efi::MemoryAttribute)(attributes | other.attributes);
        descriptor.physicalStart = std::max(address, other.physicalStart);
        descriptor.numberOfPages = std::min(endPage, otherEndPage) - (descriptor.physicalStart >> mtl::MemoryPageShift);

        // Note: the following calls to SetMemoryRange() will invalidate "descriptor" as
        // the vector storage can be reallocated to a different address.

        // Handle left piece
        if (startPage < otherStartPage)
        {
            SetMemoryRange(startPage << mtl::MemoryPageShift, otherStartPage - startPage, type, attributes);
        }
        else if (otherStartPage < startPage)
        {
            SetMemoryRange(otherStartPage << mtl::MemoryPageShift, startPage - otherStartPage, other.type, other.attributes);
        }

        // Handle right piece
        if (endPage < otherEndPage)
        {
            SetMemoryRange(endPage << mtl::MemoryPageShift, otherEndPage - endPage, other.type, other.attributes);
        }
        else if (otherEndPage < endPage)
        {
            SetMemoryRange(otherEndPage << mtl::MemoryPageShift, endPage - otherEndPage, type, attributes);
        }

        return;
    }

    // Try to merge with an existing entry
    for (auto& descriptor : m_descriptors)
    {
        if (type != descriptor.type || attributes != descriptor.attributes)
            continue;

        // Is the entry adjacent?
        if (address == descriptor.physicalStart + descriptor.numberOfPages * mtl::MemoryPageSize)
        {
            descriptor.numberOfPages += numberOfPages;
            return;
        }

        if (address + numberOfPages * mtl::MemoryPageSize == descriptor.physicalStart)
        {
            descriptor.physicalStart = address;
            descriptor.numberOfPages += numberOfPages;
            return;
        }
    }

    m_descriptors.emplace_back(efi::MemoryDescriptor{.type = type,
                                                     .padding = 0,
                                                     .physicalStart = address,
                                                     .virtualStart = 0, // TODO: is this right?
                                                     .numberOfPages = numberOfPages,
                                                     .attributes = attributes});
}

void MemoryMap::TidyUp()
{
    if (m_descriptors.size() < 2)
        return;

    // Sort entries so that we can process them in order
    std::sort(
        m_descriptors.begin(), m_descriptors.end(),
        [](const efi::MemoryDescriptor& a, const efi::MemoryDescriptor& b) -> bool { return a.physicalStart < b.physicalStart; });

    size_t lastIndex = 0;
    for (size_t i = 1; i != m_descriptors.size(); ++i)
    {
        const auto& descriptor = m_descriptors[i];

        // See if we can merge the descriptor with the last entry
        auto& last = m_descriptors[lastIndex];
        if (descriptor.type == last.type && descriptor.attributes == last.attributes &&
            descriptor.physicalStart == last.physicalStart + last.numberOfPages * mtl::MemoryPageSize)
        {
            // Extend last entry instead of creating a new one
            last.numberOfPages += descriptor.numberOfPages;
            continue;
        }

        m_descriptors[++lastIndex] = descriptor;
    }

    m_descriptors.resize(lastIndex + 1);
}
