/*
    Copyright (c) 2015, Thierry Tremblay
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
#include <stdio.h>



#if defined(__i386__) || defined(__x86_64__)
#define MEMORYZONE_LOW 0ull
#define MEMORYZONE_ISA 0x00100000ull
#define MEMORYZONE_NORMAL 0x01000000ull
#define MEMORYZONE_HIGH 0x100000000ull
#endif



MemoryMap::MemoryMap()
{
    m_count = 0;
}



void MemoryMap::AddEntry(MemoryType type, physaddr_t start, physaddr_t end)
{
    // Make sure we don't overflow
    if (end > MEMORY_MAX_PHYSICAL_ADDRESS)
        end = MEMORY_MAX_PHYSICAL_ADDRESS;

    // Ignore invalid entries (including zero-sized ones)
    if (start >= end)
        return;

    // Round to page boundaries
    if (type == MemoryType_Available)
    {
        start = MEMORY_ROUND_PAGE_UP(start);
        end = MEMORY_ROUND_PAGE_DOWN(end);
    }
    else
    {
        start = MEMORY_ROUND_PAGE_DOWN(start);
        end = MEMORY_ROUND_PAGE_UP(end);
    }

    AddEntryHelper(type, start, end);
}



void MemoryMap::AddEntryHelper(MemoryType type, physaddr_t start, physaddr_t end)
{
    // Ignore invalid entries (including zero-sized ones)
    if (start >= end)
        return;

    // Walk through our existing entries to decide what to do with this new range
    for (int i = 0; i != m_count; ++i)
    {
        MemoryEntry* entry = &m_entries[i];

        // Same type?
        if (type == entry->type)
        {
            // Check for overlaps / adjacency
            if (start <= entry->end && end >= entry->start)
            {
                // Update existing entry in-place
                if (start < entry->start)
                    entry->start = start;

                if (end > entry->end)
                    entry->end = end;

                return;
            }
        }
        else
        {
            // Types are different, check for overlaps
            if (start < entry->end && end > entry->start)
            {
                // Copy the entry as we will delete it
                MemoryEntry other = *entry;

                // Delete existing entry
                --m_count;
                for (int j = i; j != m_count; ++j)
                {
                    m_entries[j] = m_entries[j+1];
                }

                // Handle left piece
                if (start < other.start)
                    AddEntry(type, start, other.start);
                else if (other.start < start)
                    AddEntry(other.type, other.start, start);

                // Handle overlap
                MemoryType overlapType = type < other.type ? other.type : type;
                physaddr_t overlapStart = start < other.start ? other.start : start;
                physaddr_t overlapEnd = end < other.end ? end : other.end;
                AddEntry(overlapType, overlapStart, overlapEnd);

                // Handle right piece
                if (end < other.end)
                    AddEntry(other.type, end, other.end);
                else if (other.end < end)
                    AddEntry(type, other.end, end);

                return;
            }
        }
    }

    // If the table is full, we can't add more entries
    if (m_count == MEMORY_MAX_ENTRIES)
        return;

    // Insert this new entry
    MemoryEntry* entry = &m_entries[m_count];
    entry->start = start;
    entry->end = end;
    entry->type = type;
    ++m_count;
}



physaddr_t MemoryMap::AllocInRange(MemoryType type, uintptr_t sizeInBytes, physaddr_t minAddress, physaddr_t maxAddress)
{
    const uintptr_t size = MEMORY_ROUND_PAGE_UP(sizeInBytes);

    for (int i = 0; i != m_count; ++i)
    {
        const MemoryEntry& entry = m_entries[i];

        if (entry.type != MemoryType_Available)
            continue;

        // Calculate entry's overlap with what we need
        const physaddr_t overlapStart = entry.start < minAddress ? minAddress : entry.start;
        const physaddr_t overlapEnd = entry.end > maxAddress ? maxAddress : entry.end;

        if (overlapStart > overlapEnd || overlapEnd - overlapStart < size)
            continue;

        AddEntry(type, overlapStart, overlapStart + size);

        return overlapStart;
    }

    return -1;
}



physaddr_t MemoryMap::Alloc(MemoryZone zone, MemoryType type, uintptr_t sizeInBytes)
{
    if (zone < 0 || zone > MemoryZone_High)
        zone = MemoryZone_Normal;

    physaddr_t maxAddress;

    switch (zone)
    {
    case MemoryZone_Low:
        maxAddress = MEMORYZONE_ISA;
        break;

    case MemoryZone_ISA:
        maxAddress = MEMORYZONE_NORMAL;
        break;

    case MemoryZone_Normal:
        maxAddress = MEMORYZONE_HIGH;
        break;

    case MemoryZone_High:
        maxAddress = MEMORY_MAX_PHYSICAL_ADDRESS;
        break;
    }

    for ( ; zone >= 0; zone = (MemoryZone)(zone - 1))
    {
        physaddr_t minAddress;

        switch (zone)
        {
        case MemoryZone_Low:
            minAddress = MEMORYZONE_LOW;
            break;

        case MemoryZone_ISA:
            minAddress = MEMORYZONE_ISA;
            break;

        case MemoryZone_Normal:
            minAddress = MEMORYZONE_NORMAL;
            break;

        case MemoryZone_High:
            minAddress = MEMORYZONE_HIGH;
            break;
        }

        physaddr_t memory = AllocInRange(type, sizeInBytes, minAddress, maxAddress);

        if (memory != (physaddr_t)-1)
        {
            return memory;
        }
    }

    // Couldn't allocate memory
    return -1;
}



void MemoryMap::Print()
{
    printf("Memory map:\n");

    for (int i = 0; i != m_count; ++i)
    {
        const MemoryEntry& entry = m_entries[i];

        const char* type = "Unknown";

        switch (entry.type)
        {
        case MemoryType_Available:
            type = "Available";
            break;

        case MemoryType_Reserved:
            type = "Reserved";
            break;

        case MemoryType_Unusable:
            type = "Unusable";
            break;

        case MemoryType_FirmwareRuntime:
            type = "Firmware Runtime";
            break;

        case MemoryType_AcpiReclaimable:
            type = "ACPI Reclaimable";
            break;

        case MemoryType_AcpiNvs:
            type = "ACPI Non-Volatile Storage";
            break;

        case MemoryType_Bootloader:
            type = "Bootloader";
            break;
        }

        printf("    %016llx - %016llx : %s\n", entry.start, entry.end, type);
    }
}



void MemoryMap::Sanitize()
{
    // We will sanitize the memory map by doing an insert-sort of all entries
    // MemoryMap::AddEntry() will take care of merging adjacent blocks.
    MemoryMap sorted;

    while (m_count > 0)
    {
        MemoryEntry* candidate = &m_entries[0];

        for (int i = 1; i != m_count; ++i)
        {
            MemoryEntry* entry = &m_entries[i];
            if (entry->start < candidate->start)
                candidate = entry;
            else if (entry->start == candidate->start && entry->end < candidate->end)
                candidate = entry;
        }

        sorted.AddEntry(candidate->type, candidate->start, candidate->end);

        *candidate = m_entries[--m_count];
    }

    *this = sorted;
}
