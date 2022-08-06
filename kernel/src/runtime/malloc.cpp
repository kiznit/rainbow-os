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

#include <array>
#include <cstddef>
#include <metal/arch.hpp>
#include <metal/helpers.hpp>
#include <metal/log.hpp>

// Configuration
#define HAVE_MORECORE 0
#define LACKS_ERRNO_H 1
#define LACKS_FCNTL_H 1
#define LACKS_SYS_MMAN_H 1
#define LACKS_SYS_TYPES_H 1
#define LACKS_TIME_H 1
#define LACKS_UNISTD_H 1
#define MMAP_CLEARS 0

#define NO_MALLOC_STATS 1
#define USE_LOCKS 0
#define malloc_getpagesize mtl::MemoryPageSize

#define MALLOC_FAILURE_ACTION

// Fake errno.h implementation
#define EINVAL 1
#define ENOMEM 2

// Fake mman.h implementation
#define MAP_PRIVATE 2
#define MAP_ANONYMOUS 4
#define MAP_FAILED ((void*)(~(size_t)0))
#define PROT_READ 1
#define PROT_WRITE 2

// Early heap - we use this memory area before memory management is up and running.
static char s_early_heap[256 * 1024] __attribute__((aligned(4096)));
static char* s_early_heap_break = s_early_heap;
constexpr auto early_heap_end = s_early_heap + std::ssize(s_early_heap);

static void* mmap(void* address, size_t length, int prot, int flags, int fd, int offset)
{
    if (address != nullptr)
        return MAP_FAILED;

    if (prot != (PROT_READ | PROT_WRITE))
        return MAP_FAILED;

    if (flags != (MAP_ANONYMOUS | MAP_PRIVATE))
        return MAP_FAILED;

    if (length == 0 || fd != -1 || offset != 0)
        return MAP_FAILED;

    // TODO: this is not thread-safe
    if (s_early_heap_break + length <= early_heap_end)
    {
        void* p = s_early_heap_break;
        s_early_heap_break += length;
        return p;
    }

    // TODO

    MTL_LOG(Fatal) << "Out of memory";
    std::abort();
}

static int munmap(void* memory, size_t length)
{
    // TODO
    (void)memory;
    (void)length;

    return 0;
}

// Arithmetic on a null pointer treated as a cast from integer to pointer is a GNU extension
#if defined(__clang__)
#pragma clang diagnostic ignored "-Wnull-pointer-arithmetic"
#endif

#include <dlmalloc/dlmalloc.inc>
