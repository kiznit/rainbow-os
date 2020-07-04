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

#include <stdlib.h>
#include <stdint.h>
#include <malloc.h>
#include <sys/mman.h>
#include <sys/user.h>
#include "lock.h"


// TODO: we don't want to keep dlmalloc, but this will do for now

// We will now include Doug Lea's Malloc
// This provides us with malloc(), calloc(), realloc(), free() and so on...

// If you are using a hosted compiler, it might define a few things that get in the way...
#undef WIN32
#undef _WIN32
#undef linux

// TODO: as headers are added, remove these defines
#define LACKS_FCNTL_H 1
#define LACKS_SCHED_H 1
#define LACKS_SYS_MMAN_H 1
#define LACKS_SYS_TYPES_H 1
#define LACKS_TIME_H 1
#define LACKS_UNISTD_H 1


// Configuration
#define HAVE_MORECORE 0
#define MMAP_CLEARS 0
#define STRUCT_MALLINFO_DECLARED 1

#define malloc_getpagesize PAGE_SIZE

// TODO: implement locks
#define USE_LOCKS 2

// Define our own locks
#define MLOCK_T lock_t
#define INITIAL_LOCK(mutex) (*mutex = 0)
#define DESTROY_LOCK(mutex) (void)0
#define ACQUIRE_LOCK(mutex) _lock(mutex)
#define RELEASE_LOCK(mutex) _unlock(mutex)
#define TRY_LOCK(mutex)     _try_lock(mutex)

static MLOCK_T malloc_global_mutex = 0;


#include <dlmalloc/dlmalloc.inc>


void* aligned_alloc(size_t alignment, size_t size)
{
    if (size % alignment)
    {
        return NULL;
    }

    return memalign(alignment, size);
}
