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

#ifndef _RAINBOW_BOOT_MEMORY_HPP
#define _RAINBOW_BOOT_MEMORY_HPP

#include <stddef.h>
#include <metal/arch.hpp>
#include <rainbow/boot.hpp>


// Do not allocate memory at or above this address.
// This is where we want to load the kernel on 32 bits processors.
// TODO: determine this based on arch?
#define MAX_ALLOC_ADDRESS 0xF0000000

// Maximum number of entries in the memory map
const int MEMORY_MAX_ENTRIES = 1024;

// Value to represent errors on physical memory allocations (since 0 is valid)
const physaddr_t MEMORY_ALLOC_FAILED = -1;



struct MemoryEntry : MemoryDescriptor
{

    void Set(MemoryType type, uint32_t flags, uint64_t address, uint64_t size)
    {
        this->type = type;
        this->flags = flags;
        this->address = address;
        this->size = size;
    }

    uint64_t start() const                  { return this->address; }
    uint64_t end() const                    { return this->address + this->size; }

    void SetStart(uint64_t startAddress)    { size = end() - startAddress; this->address = startAddress; }
    void SetEnd(uint64_t endAddress)        { this->size = endAddress - this->address; }
};



class MemoryMap
{
public:

    MemoryMap();

    // Add 'size' bytes of memory starting at 'start' address
    void AddBytes(MemoryType type, uint32_t flags, uint64_t address, uint64_t bytesCount);

    // Allocate bytes or pages. Maximum address is optional.
    // Returns MEMORY_ALLOC_FAILED if the request can't be satisfied.
    physaddr_t AllocateBytes(MemoryType type, size_t bytesCount, uint64_t maxAddress = MAX_ALLOC_ADDRESS, uint64_t alignment = MEMORY_PAGE_SIZE);
    physaddr_t AllocatePages(MemoryType type, size_t pageCount, uint64_t maxAddress = MAX_ALLOC_ADDRESS, uint64_t alignment = MEMORY_PAGE_SIZE);


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

    void AddRange(MemoryType type, uint32_t flags, uint64_t start, uint64_t end);

    MemoryEntry m_entries[MEMORY_MAX_ENTRIES]; // Memory entries
    int         m_count;                       // Memory entry count
};



#endif
