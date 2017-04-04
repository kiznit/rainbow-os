/*
    Copyright (c) 2017, Thierry Tremblay
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
#include <inttypes.h>



// Sanity checks
static_assert(sizeof(MemoryDescriptor) == 24, "MemoryDescriptor should be packed to 24 bytes");
static_assert(sizeof(MemoryEntry) == sizeof(MemoryDescriptor), "MemoryEntry should have the same size as MemoryDescriptor");



MemoryMap::MemoryMap()
{
    m_count = 0;
}



void MemoryMap::AddBytes(MemoryType type, uint32_t flags, uint64_t address, uint64_t bytesCount)
{
    if (bytesCount == 0)
        return;

    physaddr_t start;
    physaddr_t end;

    if (type == MemoryType_Available)
    {
        start = align_up(address, MEMORY_PAGE_SIZE);

        // Check for overflow
        if (address > (physaddr_t)-1 - bytesCount + 1)
            end = 0;
        else
            end = align_down(address + bytesCount, MEMORY_PAGE_SIZE);
    }
    else
    {
        start = align_down(address, MEMORY_PAGE_SIZE);

        // Check for overflow
        if (bytesCount > align_down((physaddr_t)-1, MEMORY_PAGE_SIZE) - address)
            end = 0;
        else
            end = align_up(address + bytesCount, MEMORY_PAGE_SIZE);
    }

    AddRange(type, flags, start, end);
}



void MemoryMap::AddRange(MemoryType type, uint32_t flags, uint64_t start, uint64_t end)
{
    // Ignore invalid entries (zero-sized ones)
    if (start == end)
        return;

    // Walk all existing entries looking for overlapping ranges
    for (int i = 0; i != m_count; ++i)
    {
        MemoryEntry* entry = &m_entries[i];

        // Always check for overlaps!
        if (start < entry->end() && end > entry->start())
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
            if (start < other.start())
                AddRange(type, flags, start, other.start());
            else if (other.start() < start)
                AddRange(other.type, other.flags, other.start(), start);

            // Handle overlap
            MemoryType overlapType = type < other.type ? other.type : type;
            uint32_t overlapFlags = flags | other.flags;
            physaddr_t overlapStart = start < other.start() ? other.start() : start;
            physaddr_t overlapEnd = end < other.end() ? end : other.end();
            AddRange(overlapType, overlapFlags, overlapStart, overlapEnd);

            // Handle right piece
            if (end < other.end())
                AddRange(other.type, other.flags, end, other.end());
            else if (other.end() < end)
                AddRange(type, flags, other.end(), end);

            return;
        }
    }

    // No overlap, try to merge with an existing entry
    for (int i = 0; i != m_count; ++i)
    {
        MemoryEntry* entry = &m_entries[i];

        // Same type and flags?
        if (type != entry->type || flags != entry->flags)
            continue;

        // Check for overlaps / adjacency
        if (start <= entry->end() && end >= entry->start())
        {
            // Update existing entry in-place
            if (start < entry->start())
                entry->SetStart(start);

            if (end > entry->end())
                entry->SetEnd(end);

            return;
        }
    }

    // If the table is full, we can't add more entries
    if (m_count == MEMORY_MAX_ENTRIES)
        return;

    // Insert this new entry
    MemoryEntry* entry = &m_entries[m_count];
    entry->Set(type, flags, start, end - start);
    ++m_count;
}



physaddr_t MemoryMap::AllocateBytes(MemoryType type, size_t bytesCount, uint64_t maxAddress, uint64_t alignment)
{
    bytesCount = align_up(bytesCount, MEMORY_PAGE_SIZE);

    if (bytesCount == 0)
        return MEMORY_ALLOC_FAILED;

    const physaddr_t minAddress = MEMORY_PAGE_SIZE; //  Don't allocate NULL address
    maxAddress = align_down(maxAddress + 1, MEMORY_PAGE_SIZE);

    alignment = align_up(alignment, MEMORY_PAGE_SIZE);

    // Allocate from highest memory as possible (low memory is precious, on PC anyways)
    // To do this, we need to look at all free entries
    physaddr_t allocStart = MEMORY_ALLOC_FAILED;
    uint32_t flags = 0;

    for (int i = 0; i != m_count; ++i)
    {
        const MemoryEntry& entry = m_entries[i];

        if (entry.type != MemoryType_Available)
            continue;

        // Calculate entry's overlap with what we need
        const physaddr_t overlapStart = entry.start() < minAddress ? minAddress : entry.start();
        const physaddr_t overlapEnd = entry.end() > maxAddress ? maxAddress : entry.end();

        if (overlapStart > overlapEnd || overlapEnd - overlapStart < bytesCount)
            continue;

        const physaddr_t candidate = align_down(overlapEnd - bytesCount, alignment);

        if (allocStart == MEMORY_ALLOC_FAILED || candidate > allocStart)
        {
            allocStart = candidate;
            flags = entry.flags;
        }
    }

    if (allocStart == MEMORY_ALLOC_FAILED)
        return MEMORY_ALLOC_FAILED;

    AddRange(type, flags, allocStart, allocStart + bytesCount);

    return allocStart;
}



physaddr_t MemoryMap::AllocatePages(MemoryType type, size_t pageCount, uint64_t maxAddress, uint64_t alignment)
{
    return AllocateBytes(type, pageCount * MEMORY_PAGE_SIZE, maxAddress, alignment);
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

        case MemoryType_Persistent:
            type = "Persistent";
            break;

        case MemoryType_Unusable:
            type = "Unusable";
            break;

        case MemoryType_Bootloader:
            type = "Bootloader";
            break;

        case MemoryType_Kernel:
            type = "Kernel";
            break;

        case MemoryType_AcpiReclaimable:
            type = "ACPI Reclaimable";
            break;

        case MemoryType_AcpiNvs:
            type = "ACPI Non-Volatile Storage";
            break;

        case MemoryType_Firmware:
            type = "Firmware Runtime";
            break;

        case MemoryType_Reserved:
            type = "Reserved";
            break;
        }

        printf("    %016" PRIx64 " - %016" PRIx64 " : %s\n", entry.start(), entry.end(), type);
    }
}



static MemoryMap sorted;    // Not on stack because it is big!

void MemoryMap::Sanitize()
{
    // We will sanitize the memory map by doing an insert-sort of all entries
    // MemoryMap::AddRange() will take care of merging adjacent blocks.
    sorted.m_count = 0;

    while (m_count > 0)
    {
        MemoryEntry* candidate = &m_entries[0];

        for (int i = 1; i != m_count; ++i)
        {
            MemoryEntry* entry = &m_entries[i];
            if (entry->start() < candidate->start())
                candidate = entry;
            else if (entry->start() == candidate->start() && entry->end() < candidate->end())
                candidate = entry;
        }

        sorted.AddRange(candidate->type, candidate->flags, candidate->start(), candidate->end());

        *candidate = m_entries[--m_count];
    }

    *this = sorted;
}
