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

#include <rainbow/types.h>


#define MEMORY_MAX_ENTRIES 1024

#if defined(__i386__) || defined(__x86_64__)
#define MEMORY_PAGE_SHIFT 12
#define MEMORY_PAGE_SIZE 4096ull
#endif

#define MEMORY_ROUND_PAGE_DOWN(x) ((x) & ~(MEMORY_PAGE_SIZE - 1))
#define MEMORY_ROUND_PAGE_UP(x) (((x) + MEMORY_PAGE_SIZE - 1) & ~(MEMORY_PAGE_SIZE - 1))

// Value to represent errors on physical memory allocations (since 0 is valid)
#define MEMORY_ALLOC_FAILED ((physaddr_t)-1)


// The order these memory types are defined is important!
// When handling overlapping memory ranges, higher values take precedence.
enum MemoryType
{
    MemoryType_Available,       // Available memory (RAM)
    MemoryType_Unusable,        // Memory in which errors have been detected
    MemoryType_Bootloader,      // Bootloader
    MemoryType_Kernel,          // Kernel
    MemoryType_AcpiReclaimable, // ACPI Tables (can be reclaimed once parsed)
    MemoryType_AcpiNvs,         // ACPI Non-Volatile Storage
    MemoryType_FirmwareRuntime, // Firmware Runtime Memory (e.g. EFI runtime services)
    MemoryType_Reserved,        // Reserved / unknown / do not use
};



struct MemoryEntry
{
    physaddr_t pageStart;   // Start of memory range in pages (inclusive)
    physaddr_t pageEnd;     // End of memory range in pages (exclusive)
    MemoryType type;        // Type of memory


    // Helpers useful for unit tests mostly
    physaddr_t address() const      { return pageStart << MEMORY_PAGE_SHIFT; }
    physaddr_t pageCount() const    { return pageEnd - pageStart; }
};



class MemoryMap
{
public:

    MemoryMap();

    // Add bytes or pages of memory at the specified address.
    void AddBytes(MemoryType type, physaddr_t address, physaddr_t bytesCount);
    void AddPages(MemoryType type, physaddr_t address, physaddr_t pageCount);

    // Allocate bytes or pages. Maximum address is optional.
    // Returns MEMORY_ALLOC_FAILED if the request can't be satisfied.
    physaddr_t AllocateBytes(MemoryType type, size_t bytesCount, physaddr_t maxAddress = (1ull << 32));
    physaddr_t AllocatePages(MemoryType type, size_t pageCount, physaddr_t maxAddress = (1ull << 32));


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

    void AddPageRange(MemoryType type, physaddr_t pageStart, physaddr_t pageEnd);

    MemoryEntry  m_entries[MEMORY_MAX_ENTRIES]; // Memory entries
    int          m_count;                       // Memory entry count
};



#endif
