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

#include "boot.hpp"


extern "C" void abort()
{
    Fatal("abort()");
}



// We will now include Doug Lea's Malloc
// This provides us with malloc(), calloc(), realloc(), free() and so on...

// If you are using a hosted compiler, they might define a few things that get in the way...
#undef WIN32
#undef _WIN32
#undef errno
#undef linux

// We can't use any headers from a hosted compiler
#define LACKS_ERRNO_H 1
#define LACKS_FCNTL_H 1
#define LACKS_SCHED_H 1
#define LACKS_STDLIB_H 1
#define LACKS_STRING_H 1
#define LACKS_STRINGS_H 1
#define LACKS_SYS_MMAN_H 1
#define LACKS_SYS_PARAM_H 1
#define LACKS_SYS_TYPES_H 1
#define LACKS_TIME_H 1
#define LACKS_UNISTD_H 1

// Configuration
#define NO_MALLOC_STATS 1
#define USE_LOCKS 0

#define malloc_getpagesize MEMORY_PAGE_SIZE

// Fake errno implementation
#define EINVAL 21
#define ENOMEM 23

static int errno;

// Fake mman.h implementation
#define MAP_SHARED 1
#define MAP_PRIVATE 2
#define MAP_ANONYMOUS 4
#define MAP_ANON MAP_ANONYMOUS
#define MAP_FAILED ((void*)-1)
#define PROT_NONE  0
#define PROT_READ 1
#define PROT_WRITE 2
#define PROT_EXEC 4
#define HAVE_MORECORE 0
#define MMAP_CLEARS 0

typedef int64_t off_t;

static void* mmap(void* address, size_t length, int prot, int flags, int fd, off_t offset)
{
    (void)address;
    (void)prot;
    (void)flags;
    (void)offset;

    if (length == 0 || fd != -1)
    {
        errno = EINVAL;
        return MAP_FAILED;
    }

    const int pageCount = align_up(length, MEMORY_PAGE_SIZE) >> MEMORY_PAGE_SHIFT;

    void* memory = AllocatePages(pageCount);
    if (!memory)
    {
        errno = ENOMEM;
        return MAP_FAILED;
    }

    return memory;
}


static int munmap(void* memory, size_t length)
{
    // We don't actually free memory in the bootloader.
    // It's too complicated on some platforms and it doesn't really matter.
    (void)memory;
    (void)length;

    return 0;
}


#include <dlmalloc.inc>
