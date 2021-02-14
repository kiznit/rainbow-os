/*
    Copyright (c) 2021, Thierry Tremblay
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

#include <cerrno>
#include <cstring>
#include "boot.hpp"

// GCC is smart enough to optimize malloc() + memset() into calloc(). This results
// in an infinite loop when calling calloc() because it is basically implemented
// by calling malloc() + memset(). This will disable the optimization.
#pragma GCC optimize "no-optimize-strlen"

// Configuration
#define HAVE_MMAP 0
#define LACKS_TIME_H 1

#define NO_MALLOC_STATS 1
#define USE_LOCKS 0
#define malloc_getpagesize MEMORY_PAGE_SIZE


extern char __heap_start[];
extern char __heap_end[];

static const char* s_heapStart = (char*)&__heap_start;
static const char* s_heapEnd   = (char*)&__heap_end;
static const char* s_heapNext  = s_heapStart;


extern "C" void* sbrk(ptrdiff_t size)
{
    if (s_heapNext + size <= s_heapEnd)
    {
        auto p = (void*)s_heapNext;

        if (size > 0)
        {
            memset(p, 0, size);
        }

        s_heapNext += size;
        return p;
    }
    else
    {
        Fatal("Out of memory");
    }
}


#include <dlmalloc/dlmalloc.inc>
