/*
    Copyright (c) 2016, Thierry Tremblay
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
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>


static const physaddr_t PAGE_MAX = (((physaddr_t)-1) >> MEMORY_PAGE_SHIFT) + 1;



MemoryMap::MemoryMap()
{
    m_count = 0;
}



void MemoryMap::AddBytes(MemoryType type, physaddr_t address, physaddr_t bytesCount)
{
    if (bytesCount == 0)
        return;

    physaddr_t pageStart;
    physaddr_t pageEnd;

    if (type == MemoryType_Available)
    {
        // Calculate start page
        pageStart = address >> MEMORY_PAGE_SHIFT; // 0..PAGE_MAX-1

        // Round start address up to the next page boundary
        physaddr_t delta = address & (MEMORY_PAGE_SIZE-1);
        if (delta > 0)
        {
            ++pageStart; // 0..PAGE_MAX

            // Check if we have enough in bytesCount to compensate for the page rounding
            delta = MEMORY_PAGE_SIZE - delta;
            if (delta >= bytesCount)
                return;

            // Fix bytes count
            bytesCount = bytesCount - delta;
        }

        // Calculate end page (rounding down)
        pageEnd = pageStart + (bytesCount >> MEMORY_PAGE_SHIFT); // 0..PAGE_MAX*2-1
    }
    else
    {
        // Calculate start page (rounded down) and end page (rounded up)
        pageStart = address >> MEMORY_PAGE_SHIFT; // 0..PAGE_MAX-1
        pageEnd = pageStart + (bytesCount >> MEMORY_PAGE_SHIFT); // 0..PAGE_MAX*2-1

        // Calculate how many bytes we missed with our roundings above
        physaddr_t missing = (address & (MEMORY_PAGE_SIZE-1)) + (bytesCount & (MEMORY_PAGE_SIZE-1));

        // Fix page end to account for missing bytes
        pageEnd = pageEnd + (missing >> MEMORY_PAGE_SHIFT); // 0..PAGE_MAX*2
        if (missing & (MEMORY_PAGE_SIZE-1))
            ++pageEnd; // 0..PAGE_MAX*2+1
    }

    if (pageEnd > PAGE_MAX)
        pageEnd = PAGE_MAX; // 0..PAGE_MAX

    AddPageRange(type, pageStart, pageEnd);
}



void MemoryMap::AddPages(MemoryType type, physaddr_t address, physaddr_t pageCount)
{
    physaddr_t pageStart;
    physaddr_t pageEnd;

    // Limit pageCount to some reasonable number to prevent overflows
    if (pageCount > PAGE_MAX + 2)
        pageCount = PAGE_MAX + 2; // 0..PAGE_MAX+2

    // Calculate start page
    pageStart = address >> MEMORY_PAGE_SHIFT; // 0..PAGE_MAX-1

    if (type == MemoryType_Available)
    {
        if (address & (MEMORY_PAGE_SIZE-1))
        {
            if (pageCount < 2)
                return;

            pageStart += 1; // 0..PAGE_MAX
            pageCount -= 2; // 0..PAGE_MAX
        }
    }
    else
    {
        // Calculate start page
        pageStart = address >> MEMORY_PAGE_SHIFT; // 0..PAGE_MAX-1
    }

    // Calculate end page
    pageEnd = pageStart + pageCount; // 0..PAGE_MAX*2+2

    // Overflow check
    if (pageEnd > PAGE_MAX)
        pageEnd = PAGE_MAX; // 0..PAGE_MAX

    AddPageRange(type, pageStart, pageEnd);
}



void MemoryMap::AddPageRange(MemoryType type, physaddr_t pageStart, physaddr_t pageEnd)
{
    // Ignore invalid entries (including zero-sized ones)
    if (pageStart >= pageEnd)
        return;

    // Walk through our existing entries to decide what to do with this new range
    for (int i = 0; i != m_count; ++i)
    {
        MemoryEntry* entry = &m_entries[i];

        // Same type?
        if (type == entry->type)
        {
            // Check for overlaps / adjacency
            if (pageStart <= entry->pageEnd && pageEnd >= entry->pageStart)
            {
                // Update existing entry in-place
                if (pageStart < entry->pageStart)
                    entry->pageStart = pageStart;

                if (pageEnd > entry->pageEnd)
                    entry->pageEnd = pageEnd;

                return;
            }
        }
        else
        {
            // Types are different, check for overlaps
            if (pageStart < entry->pageEnd && pageEnd > entry->pageStart)
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
                if (pageStart < other.pageStart)
                    AddPageRange(type, pageStart, other.pageStart);
                else if (other.pageStart < pageStart)
                    AddPageRange(other.type, other.pageStart, pageStart);

                // Handle overlap
                MemoryType overlapType = type < other.type ? other.type : type;
                physaddr_t overlapStart = pageStart < other.pageStart ? other.pageStart : pageStart;
                physaddr_t overlapEnd = pageEnd < other.pageEnd ? pageEnd : other.pageEnd;
                AddPageRange(overlapType, overlapStart, overlapEnd);

                // Handle right piece
                if (pageEnd < other.pageEnd)
                    AddPageRange(other.type, pageEnd, other.pageEnd);
                else if (other.pageEnd < pageEnd)
                    AddPageRange(type, other.pageEnd, pageEnd);

                return;
            }
        }
    }

    // If the table is full, we can't add more entries
    if (m_count == MEMORY_MAX_ENTRIES)
        return;

    // Insert this new entry
    MemoryEntry* entry = &m_entries[m_count];
    entry->pageStart = pageStart;
    entry->pageEnd = pageEnd;
    entry->type = type;
    ++m_count;
}



physaddr_t MemoryMap::AllocatePages(MemoryType type, size_t pageCount, physaddr_t maxAddress)
{
    const physaddr_t minPage = 1;  //  Don't allocate NULL address

    if (!maxAddress)
        maxAddress = (physaddr_t)-1;

    physaddr_t maxPage = maxAddress >> MEMORY_PAGE_SHIFT;
    if (maxAddress & (MEMORY_PAGE_SIZE - 1))
        ++maxPage;


    // Allocate from highest memory as possible (low memory is precious, on PC anyways)
    for (int i = m_count; i != 0; --i)
    {
        const MemoryEntry& entry = m_entries[i-1];

        if (entry.type != MemoryType_Available)
            continue;

        // Calculate entry's overlap with what we need
        const physaddr_t overlapStart = entry.pageStart < minPage ? minPage : entry.pageStart;
        const physaddr_t overlapEnd = entry.pageEnd > maxPage ? maxPage : entry.pageEnd;

        if (overlapStart > overlapEnd || overlapEnd - overlapStart < pageCount)
            continue;

        const physaddr_t allocStart = overlapEnd - pageCount;
        const physaddr_t allocEnd = overlapEnd;

        AddPageRange(type, allocStart, allocEnd);

        return allocStart << MEMORY_PAGE_SHIFT;
    }

    return (physaddr_t)-1;
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

        case MemoryType_BootModule:
            type = "Boot Module";
            break;

        case MemoryType_Launcher:
            type = "Launcher";
            break;
        }

        printf("    %016" PRIx64 " - %016" PRIx64 " : %s\n",
            entry.pageStart << MEMORY_PAGE_SHIFT,
            entry.pageEnd << MEMORY_PAGE_SHIFT,
            type);
    }
}



void MemoryMap::Sanitize()
{
    // We will sanitize the memory map by doing an insert-sort of all entries
    // MemoryMap::AddPageRange() will take care of merging adjacent blocks.
    MemoryMap sorted;

    while (m_count > 0)
    {
        MemoryEntry* candidate = &m_entries[0];

        for (int i = 1; i != m_count; ++i)
        {
            MemoryEntry* entry = &m_entries[i];
            if (entry->pageStart < candidate->pageStart)
                candidate = entry;
            else if (entry->pageStart == candidate->pageStart && entry->pageEnd < candidate->pageEnd)
                candidate = entry;
        }

        sorted.AddPageRange(candidate->type, candidate->pageStart, candidate->pageEnd);

        *candidate = m_entries[--m_count];
    }

    *this = sorted;
}
