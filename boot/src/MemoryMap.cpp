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

MemoryMap::MemoryMap(std::vector<MemoryDescriptor>&& descriptors)
    : m_descriptors(std::move(descriptors))
{
    TidyUp();
}

std::expected<PhysicalAddress, bool> MemoryMap::AllocatePages(MemoryType memoryType,
                                                              size_t pageCount)
{
    assert(memoryType != MemoryType::Available);
    assert(pageCount > 0);

    // Allocate from highest available memory (low addresses are precious, on PC anyways).
    MemoryDescriptor* candidate{};

    for (auto& descriptor : m_descriptors)
    {
        if (descriptor.type != MemoryType::Available)
            continue;

        if (descriptor.pageCount < pageCount)
            continue;

        if (!candidate || descriptor.address > candidate->address)
            candidate = &descriptor;
    }

    if (!candidate)
        return std::unexpected(false);

    if (candidate->pageCount == pageCount)
    {
        candidate->type = memoryType;
        return candidate->address;
    }
    else
    {
        // We have to be careful about recursion here. Calling SetMemoryRange() can grow the vector
        // of descriptors, which means that AllocatePages() could be called recursively. Consider
        // what would happen if:
        //      1) 'candidate' is the only block of 'Available' memory and
        //      2) the vector needs to grow

        const PhysicalAddress memory = candidate->address;

        candidate->address += pageCount * mtl::MEMORY_PAGE_SIZE;
        candidate->pageCount -= pageCount;

        SetMemoryRange(memory, pageCount, memoryType, candidate->flags);

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
        case MemoryType::Available:
            type = u8"Available";
            break;

        case MemoryType::Unusable:
            type = u8"Unusable";
            break;

        case MemoryType::Bootloader:
            type = u8"Bootloader";
            break;

        case MemoryType::KernelCode:
            type = u8"Kernel Code";
            break;

        case MemoryType::KernelData:
            type = u8"Kernel Data";
            break;

        case MemoryType::AcpiReclaimable:
            type = u8"ACPI Reclaimable";
            break;

        case MemoryType::AcpiNvs:
            type = u8"ACPI Non-Volatile";
            break;

        case MemoryType::UefiCode:
            type = u8"UEFI Code";
            break;

        case MemoryType::UefiData:
            type = u8"UEFI Data";
            break;

        case MemoryType::Persistent:
            type = u8"Persistent";
            break;

        case MemoryType::Reserved:
            type = u8"Reserved";
            break;
        }
        MTL_LOG(Info) << "    " << mtl::hex(descriptor.address) << " - "
                      << mtl::hex(descriptor.address +
                                  descriptor.pageCount * mtl::MEMORY_PAGE_SIZE - 1)
                      << ": " << mtl::hex(descriptor.flags) << " " << type;
    }
}

void MemoryMap::SetMemoryRange(PhysicalAddress address, uint64_t pageCount, MemoryType type,
                               MemoryFlags flags)
{
    assert(mtl::is_aligned(address, mtl::MEMORY_PAGE_SIZE));

    if (!pageCount)
        return;

    // We will do computations using page numbers instead of addresses. The main reason for this
    // is so that we don't have to deal with tricky overflow arithmetics when computing the end
    // address of a range at the end of memory space.
    const uint64_t startPage = address >> mtl::MEMORY_PAGE_SHIFT;
    const uint64_t endPage = startPage + pageCount;

    // Walk all existing entries looking for overlapping ranges
    for (auto& descriptor : m_descriptors)
    {
        const uint64_t otherStartPage = descriptor.address >> mtl::MEMORY_PAGE_SHIFT;
        const uint64_t otherEndPage = otherStartPage + descriptor.pageCount;

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
        descriptor.flags = (MemoryFlags)(flags | other.flags);
        descriptor.address = std::max(address, other.address);
        descriptor.pageCount =
            std::min(endPage, otherEndPage) - (descriptor.address >> mtl::MEMORY_PAGE_SHIFT);

        // Note: the following calls to SetMemoryRange() will invalidate "descriptor" as
        // the vector storage can be reallocated to a different address.

        // Handle left piece
        if (startPage < otherStartPage)
        {
            SetMemoryRange(startPage << mtl::MEMORY_PAGE_SHIFT, otherStartPage - startPage, type,
                           flags);
        }
        else if (otherStartPage < startPage)
        {
            SetMemoryRange(otherStartPage << mtl::MEMORY_PAGE_SHIFT, startPage - otherStartPage,
                           other.type, other.flags);
        }

        // Handle right piece
        if (endPage < otherEndPage)
        {
            SetMemoryRange(endPage << mtl::MEMORY_PAGE_SHIFT, otherEndPage - endPage, other.type,
                           other.flags);
        }
        else if (otherEndPage < endPage)
        {
            SetMemoryRange(otherEndPage << mtl::MEMORY_PAGE_SHIFT, endPage - otherEndPage, type,
                           flags);
        }

        return;
    }

    // Try to merge with an existing entry
    for (auto& descriptor : m_descriptors)
    {
        if (type != descriptor.type || flags != descriptor.flags)
            continue;

        // Is the entry adjacent?
        if (address == descriptor.address + descriptor.pageCount * mtl::MEMORY_PAGE_SIZE)
        {
            descriptor.pageCount += pageCount;
            return;
        }

        if (address + pageCount * mtl::MEMORY_PAGE_SIZE == descriptor.address)
        {
            descriptor.address = address;
            descriptor.pageCount += pageCount;
            return;
        }
    }

    m_descriptors.emplace_back(
        MemoryDescriptor{.type = type, .flags = flags, .address = address, .pageCount = pageCount});
}

void MemoryMap::TidyUp()
{
    if (m_descriptors.size() < 2)
        return;

    // Sort entries so that we can process them in order
    std::sort(m_descriptors.begin(), m_descriptors.end(),
              [](const MemoryDescriptor& a, const MemoryDescriptor& b) -> bool {
                  return a.address < b.address;
              });

    size_t lastIndex = 0;
    for (size_t i = 1; i != m_descriptors.size(); ++i)
    {
        const auto& descriptor = m_descriptors[i];

        // See if we can merge the descriptor with the last entry
        auto& last = m_descriptors[lastIndex];
        if (descriptor.type == last.type && descriptor.flags == last.flags &&
            descriptor.address == last.address + last.pageCount * mtl::MEMORY_PAGE_SIZE)
        {
            // Extend last entry instead of creating a new one
            last.pageCount += descriptor.pageCount;
            continue;
        }

        m_descriptors[++lastIndex] = descriptor;
    }

    m_descriptors.resize(lastIndex + 1);
}
