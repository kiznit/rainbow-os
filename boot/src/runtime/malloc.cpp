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

#include "boot.hpp"
#include <cstdlib>
#include <metal/exception.hpp>
#include <metal/helpers.hpp>

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

    const auto pageCount = mtl::align_up(length, mtl::MemoryPageSize) >> mtl::MemoryPageShift;

    const auto memory = AllocatePages(pageCount);
    if (!memory)
        return MAP_FAILED;

    return reinterpret_cast<void*>(*memory);
}

static int munmap(void* memory, size_t length)
{
    // We don't free memory in the bootloader, it doesn't matter.
    (void)memory;
    (void)length;

    return 0;
}

// Some compilers will define _MSC_VER/_WIN32/WIN32 when targetting UEFI.
// We do not want this with dlmalloc.
#undef _MSC_VER
#undef _WIN32
#undef WIN32

// Arithmetic on a null pointer treated as a cast from integer to pointer is a GNU extension
#if defined(__clang__)
#pragma clang diagnostic ignored "-Wnull-pointer-arithmetic"
#endif

#include <dlmalloc/dlmalloc.inc>
