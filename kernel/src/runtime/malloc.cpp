/*
    Copyright (c) 2022, Thierry Tremblay
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

// TODO: do not use dlmalloc
// TODO: use one pool per CPU

#include "memory.hpp"
#include <array>
#include <cstddef>
#include <metal/arch.hpp>
#include <metal/helpers.hpp>
#include <metal/log.hpp>

#if !defined(__clang__)
// GCC is smart enough to optimize malloc() + memset() into calloc(). This results
// in an infinite loop when calling calloc() because it is basically implemented
// by calling malloc() + memset(). This will disable the optimization.
#pragma GCC optimize "no-optimize-strlen"
#endif

// Configuration
#define HAVE_MMAP 0
#define LACKS_TIME_H 1
#define LACKS_ERRNO_H 1
#define LACKS_SYS_TYPES_H 1
#define LACKS_UNISTD_H 1

#define NO_MALLOC_STATS 1
// TODO: use locks, we need malloc() to be thread-safe
#define USE_LOCKS 0
#define malloc_getpagesize mtl::kMemoryPageSize

#define MORECORE dlmalloc_sbrk
#define MALLOC_FAILURE_ACTION

// Fake errno.h implementation
#define EINVAL 1
#define ENOMEM 2

extern "C" char __heap_start[];
extern "C" char __heap_end[];

namespace
{
    char* const g_heapStart = __heap_start; // Start of heap
    char* g_heapEnd = __heap_end;           // End of allocated virtual memory for the heap
    char* g_heapBreak = __heap_start;       // Current heap break
} // namespace

// Note: this function is not thread safe. Concurrency relies on dlmallloc using locks.
void* dlmalloc_sbrk(ptrdiff_t size)
{
    assert(mtl::IsAligned(g_heapStart, mtl::kMemoryPageSize));
    assert(mtl::IsAligned(g_heapEnd, mtl::kMemoryPageSize));

    const auto newBreak = g_heapBreak + size;

    if (g_heapBreak < g_heapStart)
        return (void*)-1;

    // Do we need to map more memory?
    if (newBreak > g_heapEnd)
    {
        // Calculate how much we need to allocate
        const auto mapSize = mtl::AlignUp(newBreak, mtl::kMemoryPageSize) - g_heapEnd;

        if (!VirtualAlloc(g_heapEnd, mapSize))
        {
            MTL_LOG(Fatal) << "Out of memory";
            std::abort();
        }

        g_heapEnd += mapSize;
    }
    else if (size < 0)
    {
        // Calculate how much we can free
        const auto mapSize = g_heapEnd - mtl::AlignUp(newBreak, mtl::kMemoryPageSize);

        if (VirtualFree(g_heapEnd - mapSize, mapSize))
            g_heapEnd -= mapSize;
    }

    void* p = g_heapBreak;
    g_heapBreak = newBreak;
    return p;
}

// Arithmetic on a null pointer treated as a cast from integer to pointer is a GNU extension
#if defined(__clang__)
#pragma clang diagnostic ignored "-Wnull-pointer-arithmetic"
#endif

#include <dlmalloc/dlmalloc.inc>
