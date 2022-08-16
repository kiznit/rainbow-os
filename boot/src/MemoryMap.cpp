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

MemoryMap::MemoryMap(std::vector<efi::MemoryDescriptor>&& descriptors) : m_descriptors(std::move(descriptors))
{
    TidyUp();
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
                      << mtl::hex(descriptor.physicalStart + descriptor.numberOfPages * mtl::kMemoryPageSize - 1) << ": "
                      << " " << type;
    }
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
            descriptor.physicalStart == last.physicalStart + last.numberOfPages * mtl::kMemoryPageSize)
        {
            // Extend last entry instead of creating a new one
            last.numberOfPages += descriptor.numberOfPages;
            continue;
        }

        m_descriptors[++lastIndex] = descriptor;
    }

    m_descriptors.resize(lastIndex + 1);
}
