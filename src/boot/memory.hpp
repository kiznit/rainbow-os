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

#include <vector>
#include <rainbow/boot.hpp>


// Do not allocate memory at or above this address.
// This is where we want to load the kernel on 32 bits processors.
// TODO: determine this based on arch?
static const physaddr_t MAX_ALLOC_ADDRESS = 0xF0000000;


struct MemoryEntry : MemoryDescriptor
{
    physaddr_t start() const                { return this->address; }
    physaddr_t end() const                  { return this->address + this->size; }

    void SetStart(physaddr_t startAddress)  { size = end() - startAddress; this->address = startAddress; }
    void SetEnd(physaddr_t endAddress)      { this->size = endAddress - this->address; }
};



class MemoryMap
{
    typedef std::vector<MemoryEntry> Storage;

public:

    // Add 'size' bytes of memory starting at 'start' address
    void AddBytes(MemoryType type, MemoryFlags flags, physaddr_t address, physaddr_t bytesCount);

    // Allocate bytes or pages. Maximum address is optional, but no memory will be allocated above MAX_ALLOC_ADDRESS.
    // Throws std::bad_alloc if the request can't be satisfied.
    physaddr_t AllocateBytes(MemoryType type, size_t bytesCount, physaddr_t maxAddress = MAX_ALLOC_ADDRESS);
    physaddr_t AllocatePages(MemoryType type, size_t pageCount, physaddr_t maxAddress = MAX_ALLOC_ADDRESS);

    void Print();

    void Sanitize();

    // Container interface
    typedef Storage::const_iterator const_iterator;

    const_iterator begin() const                    { return m_entries.begin(); }
    const_iterator end() const                      { return m_entries.end(); }
    size_t size() const                             { return m_entries.size(); }
    const MemoryEntry* data() const                 { return m_entries.data(); }

    const MemoryEntry& operator[](int index) const  { return m_entries[index]; }


private:

    void AddRange(MemoryType type, MemoryFlags flags, physaddr_t start, physaddr_t end);

    Storage m_entries;
};



#endif
