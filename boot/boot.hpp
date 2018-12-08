/*
    Copyright (c) 2018, Thierry Tremblay
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

#ifndef _RAINBOW_BOOT_HPP
#define _RAINBOW_BOOT_HPP

#include <stddef.h>
#include <stdint.h>


#define ARRAY_LENGTH(array) (sizeof(array) / sizeof((array)[0]))

#define STRINGIZE_DELAY(x) #x
#define STRINGIZE(x) STRINGIZE_DELAY(x)

extern "C"
{
    void* memcpy(void*, const void*, size_t);
    void* memset(void*, int, size_t);
}


inline void* operator new(size_t, void* p) { return p; }
inline void* operator new[](size_t, void* p) { return p; }


// Do not allocate memory at or above this address.
// This is where we want to load the kernel on 32 bits processors.
#define MAX_ALLOC_ADDRESS 0xF0000000


// The order these memory types are defined is important!
// When the firmware returns overlapping memory ranges, higher values take precedence.
enum MemoryType
{
    MemoryType_Available,           // Conventional memory (RAM)
    MemoryType_Persistent,          // Works like conventional memory, but is persistent
    MemoryType_Unusable,            // Memory in which errors have been detected
    MemoryType_Bootloader,          // Bootloader
    MemoryType_Kernel,              // Kernel
    MemoryType_AcpiReclaimable,     // ACPI Tables (can be reclaimed once parsed)
    MemoryType_AcpiNvs,             // ACPI Non-Volatile Storage
    MemoryType_Firmware,            // Firmware (e.g. EFI runtime services, ARM Device Tree, ...)
    MemoryType_Reserved,            // Reserved / unknown / do not use
};


enum MemoryFlag
{
    MemoryFlag_Code         = 1,    // Memory is code
    MemoryFlag_ReadOnly     = 2,    // Memory is read-only
};


struct MemoryDescriptor
{
    MemoryType  type;               // Memory type
    uint32_t    flags;              // Memory flags
    uint64_t    address;            // Start of memory range
    uint64_t    size;               // Size of memory range in bytes
};

static_assert(sizeof(MemoryDescriptor) == 24, "MemoryDescriptor should be packed to 24 bytes");


template<typename T>
T* advance_pointer(T* p, intptr_t delta)
{
    return (T*)((uintptr_t)p + delta);
}


template<typename T>
T* align_down(T* p, unsigned int alignment)
{
    return (T*)((uintptr_t)p & ~(alignment - 1));
}


template<typename T>
T align_down(T v, unsigned int alignment)
{
    return (v) & ~(T(alignment) - 1);
}


template<typename T>
T* align_up(T* p, unsigned int alignment)
{
    return (T*)(((uintptr_t)p + alignment - 1) & ~(alignment - 1));
}


template<typename T>
T align_up(T v, unsigned int alignment)
{
    return (v + T(alignment) - 1) & ~(T(alignment) - 1);
}


template<typename T>
const T& min(const T& a, const T& b)
{
    return a < b ? a : b;
}


template<typename T>
const T& max(const T& a, const T& b)
{
    return a < b ? b : a;
}

#endif
