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

#ifndef _RAINBOW_BOOT_MEMORY_HPP
#define _RAINBOW_BOOT_MEMORY_HPP

#include <rainbow/boot.hpp>


#define MEMORY_MAX_ENTRIES 1024

#define MEMORY_ROUND_PAGE_DOWN(x) ((x) & ~(MEMORY_PAGE_SIZE - 1))
#define MEMORY_ROUND_PAGE_UP(x) (((x) + MEMORY_PAGE_SIZE - 1) & ~(MEMORY_PAGE_SIZE - 1))

// Value to represent errors on physical memory allocations (since 0 is valid)
#define MEMORY_ALLOC_FAILED ((physaddr_t)-1)



struct MemoryEntry : MemoryDescriptor
{

    void Initialize(MemoryType type, uint32_t flags, uint64_t pageStart, uint64_t pageEnd)
    {
        this->type = type;
        this->flags = flags;
        this->address = pageStart << MEMORY_PAGE_SHIFT;
        this->numberOfPages = pageEnd - pageStart;
    }

    // start / end of range in pages (and not bytes)
    uint64_t pageStart() const          { return address >> MEMORY_PAGE_SHIFT; }
    uint64_t pageEnd() const            { return pageStart() + pageCount(); }
    uint64_t pageCount() const          { return numberOfPages; }

    void SetStart(uint64_t pageStart)   { numberOfPages = pageEnd() - pageStart; address = pageStart << MEMORY_PAGE_SHIFT; }
    void SetEnd(uint64_t pageEnd)       { numberOfPages = pageEnd - pageStart(); }
};



class MemoryMap
{
public:

    MemoryMap();

    // Add 'size' bytes of memory starting at 'start' address
    void AddBytes(MemoryType type, uint32_t flags, uint64_t address, uint64_t bytesCount);

    // Allocate bytes or pages. Maximum address is optional.
    // Returns MEMORY_ALLOC_FAILED if the request can't be satisfied.
    physaddr_t AllocateBytes(MemoryType type, size_t bytesCount, uint64_t maxAddress = 0xFFFFFFFF);
    physaddr_t AllocatePages(MemoryType type, size_t pageCount, uint64_t maxAddress = 0xFFFFFFFF);


    void Print();

    void Sanitize();

    // Container interface
    typedef const MemoryEntry* const_iterator;
    typedef const MemoryEntry& const_reference;

    void clear()                                { m_count = 0; }

    size_t size() const                         { return m_count; }
    const_iterator begin() const                { return m_entries; }
    const_iterator end() const                  { return m_entries + m_count; }

    const_reference operator[](int i) const     { return m_entries[i]; }


private:

    void AddPageRange(MemoryType type, uint32_t flags, uint64_t pageStart, uint64_t pageEnd);

    MemoryEntry m_entries[MEMORY_MAX_ENTRIES]; // Memory entries
    int         m_count;                       // Memory entry count
};



#endif
