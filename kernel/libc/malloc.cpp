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

#include <cstdio>
#include <cerrno>
#include <metal/arch.hpp>
#include <metal/helpers.hpp>
#include <kernel/spinlock.hpp>
#include <kernel/vmm.hpp>

// GCC is smart enough to optimize malloc() + memset() into calloc(). This results
// in an infinite loop when calling calloc() because it is basically implemented
// by calling malloc() + memset(). This will disable the optimization.
#pragma GCC optimize "no-optimize-strlen"


//#define LACKS_ERRNO_H 1
//#define LACKS_FCNTL_H 1
//#define LACKS_SCHED_H 1
//#define LACKS_STDLIB_H 1
//#define LACKS_STRING_H 1
//#define LACKS_STRINGS_H 1
#define LACKS_SYS_MMAN_H 1
//#define LACKS_SYS_PARAM_H 1
//#define LACKS_SYS_TYPES_H 1
#define LACKS_TIME_H 1
//#define LACKS_UNISTD_H 1

// Configuration
#define NO_MALLOC_STATS 1
#define USE_LOCKS 2
#define malloc_getpagesize MEMORY_PAGE_SIZE

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
#define MMAP_CLEARS 1

// Define our own locks
#define MLOCK_T             Spinlock
#define INITIAL_LOCK(mutex) (void)0
#define DESTROY_LOCK(mutex) (void)0
#define ACQUIRE_LOCK(mutex) ((mutex)->lock(), 0)
#define RELEASE_LOCK(mutex) (mutex)->unlock()
#define TRY_LOCK(mutex)     (mutex)->try_lock()

static MLOCK_T malloc_global_mutex;


// Early memory allocation use a static buffer.
// TODO: initialize memory management before invoking global constructors in _init()

static char s_early_memory[65536] alignas(16);  // This is how much memory dlmalloc requests at a time.
static bool s_early_memory_allocated;           // Is the early memory buffer allocated?


static void* mmap(void* address, size_t length, int prot, int flags, int fd, off_t offset) noexcept
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

    if (!s_early_memory_allocated && length <= sizeof(s_early_memory))
    {
        s_early_memory_allocated = true;
        return s_early_memory;
    }

    const int pageCount = align_up(length, MEMORY_PAGE_SIZE) >> MEMORY_PAGE_SHIFT;

    void* memory = vmm_allocate_pages(pageCount);
    if (!memory)
    {
        errno = ENOMEM;
        return MAP_FAILED;
    }

    return memory;
}


static int munmap(void* memory, size_t length) noexcept
{
    // TODO
    (void)memory;
    (void)length;

    return 0;
}


#include <dlmalloc/dlmalloc.inc>
