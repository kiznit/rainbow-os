/*
    Copyright (c) 2020, Thierry Tremblay
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
#include <algorithm>
#include <cassert>
#include <metal/helpers.hpp>
#include <metal/log.hpp>


// Sanity checks
static_assert(sizeof(MemoryDescriptor) == 24, "MemoryDescriptor should be packed to 24 bytes");
static_assert(sizeof(MemoryEntry) == sizeof(MemoryDescriptor), "MemoryEntry should have the same size as MemoryDescriptor");


void MemoryMap::AddBytes(MemoryType type, MemoryFlags flags, physaddr_t address, physaddr_t size)
{
    if (size == 0)
    {
        return;
    }

    physaddr_t start;
    physaddr_t end;

    if (type == MemoryType::Available)
    {
        start = align_up(address, MEMORY_PAGE_SIZE);

        // Check for overflow
        if (address > (physaddr_t)-1 - size + 1)
        {
            end = 0;
        }
        else
        {
            end = align_down(address + size, MEMORY_PAGE_SIZE);
        }
    }
    else
    {
        start = align_down(address, MEMORY_PAGE_SIZE);

        // Check for overflow
        if (size > align_down((physaddr_t)-1, MEMORY_PAGE_SIZE) - address)
        {
            end = 0;
        }
        else
        {
            end = align_up(address + size, MEMORY_PAGE_SIZE);
        }
    }

    AddRange(type, flags, start, end);
}



void MemoryMap::AddRange(MemoryType type, MemoryFlags flags, physaddr_t start, physaddr_t end)
{
    // Ignore invalid entries (zero-sized ones)
    if (start == end)
    {
        return;
    }

    // Walk all existing entries looking for overlapping ranges
    for (auto& entry: m_entries)
    {
        // Always check for overlaps!
        if (start < entry.end() && end > entry.start())
        {
            // Copy the entry as we will overwrite it below
            MemoryEntry other = entry;

            // Handle overlap
            auto overlapType = type < other.type ? other.type : type;
            auto overlapFlags = flags | other.flags;
            auto overlapStart = start < other.start() ? other.start() : start;
            auto overlapEnd = end < other.end() ? end : other.end();

            // Modify entry in-place
            entry.type = overlapType;
            entry.flags = overlapFlags;
            entry.address = overlapStart;
            entry.size = overlapEnd - overlapStart;

            // Note: the following calls to AddRange() will invalidate "entry" as
            // the storage can be reallocated to a different address.

            // Handle left piece
            if (start < other.start())
            {
                AddRange(type, flags, start, other.start());
            }
            else if (other.start() < start)
            {
                AddRange(other.type, other.flags, other.start(), start);
            }

            // Handle right piece
            if (end < other.end())
            {
                AddRange(other.type, other.flags, end, other.end());
            }
            else if (other.end() < end)
            {
                AddRange(type, flags, other.end(), end);
            }

            return;
        }
    }

    // Try to merge with an existing entry
    for (auto& entry: m_entries)
    {
        // Same type and flags?
        if (type != entry.type || flags != entry.flags)
        {
            continue;
        }

        // Is the entry adjacent?
        if (start == entry.end())
        {
            entry.SetEnd(end);
            return;
        }

        if (end == entry.start())
        {
            // Update existing entry in-place
            entry.SetStart(start);
            return;
        }
    }

    // Insert a new entry
    m_entries.push_back(
        MemoryEntry {{ .type = type, .flags = flags, .address = start, .size = end - start }}
    );
}



physaddr_t MemoryMap::AllocateBytes(MemoryType type, size_t size, physaddr_t maxAddress)
{
    assert(size > 0);

    size = align_up(size, MEMORY_PAGE_SIZE);

    const physaddr_t minAddress = MEMORY_PAGE_SIZE; //  Don't allocate NULL address
    maxAddress = align_down(std::min(maxAddress, MAX_ALLOC_ADDRESS), MEMORY_PAGE_SIZE);

    // Find all entries that can satisfy the allocation
    auto candidates = m_entries | std::views::filter([&](const auto& entry) {
        if (entry.type != MemoryType::Available)
        {
            return false;
        }

        // Verify minAddress and maxAddress
        if (std::max(entry.start(), minAddress) + size > std::min(entry.end(), maxAddress))
        {
            return false;
        }

        return true;
    });

    if (candidates.empty())
    {
        Fatal("Out of memory");
    }

    // Allocate from highest memory as possible (low memory is precious, on PC anyways)
    const auto& entry = std::ranges::max_element(candidates, [&](const auto& a, const auto& b) {
        return a.address < b.address;
    });

    // We already verified that we can satisfy the allocation, calculate its location
    auto end = std::min(entry->end(), maxAddress);
    auto start = end - size;

    // Update the map to mark this memory as used
    AddRange(type, entry->flags, start, end);

    return start;
}



physaddr_t MemoryMap::AllocatePages(MemoryType type, size_t pageCount, physaddr_t maxAddress)
{
    return AllocateBytes(type, pageCount * MEMORY_PAGE_SIZE, maxAddress);
}



void MemoryMap::Print()
{
    Log("Memory map:\n");

    for (auto& entry: m_entries)
    {
        const char* type = "Unknown";

        switch (entry.type)
        {
        case MemoryType::Available:
            type = "Available";
            break;

        case MemoryType::Persistent:
            type = "Persistent";
            break;

        case MemoryType::Unusable:
            type = "Unusable";
            break;

        case MemoryType::Bootloader:
            type = "Bootloader";
            break;

        case MemoryType::Kernel:
            type = "Kernel";
            break;

        case MemoryType::AcpiReclaimable:
            type = "ACPI Reclaimable";
            break;

        case MemoryType::AcpiNvs:
            type = "ACPI Non-Volatile Storage";
            break;

        case MemoryType::Firmware:
            type = "Firmware";
            break;

        case MemoryType::Reserved:
            type = "Reserved";
            break;
        }

        const char* flags = "Data";

        if (any(entry.flags & MemoryFlags::Code))
        {
            flags = "Code";
        }
        else if (any(entry.flags & MemoryFlags::ReadOnly))
        {
            flags = "ReadOnly";
        }

        Log("    %016jX - %016jX (%016jX): %-8s : %s\n", entry.start(), entry.end(), entry.end() - entry.start(), flags, type);
    }
}



void MemoryMap::Sanitize()
{
    std::ranges::sort(m_entries, [](const auto& a, const auto& b) {
        return a.address < b.address;
    });

    // Merge adjacent entries
    Storage copy;
    for (const auto& entry: m_entries)
    {
        // See if we can merge 'entry' with the last one in 'copy'
        if (!copy.empty())
        {
            auto& last = copy.back();
            if (entry.type == last.type && entry.flags == last.flags && entry.start() == last.end())
            {
                last.SetEnd(entry.end());
                continue;
            }
        }

        copy.push_back(entry);
    }

    m_entries = std::move(copy);
}
