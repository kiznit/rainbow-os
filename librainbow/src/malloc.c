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

// TODO: we shouldn't need this file, malloc() is to be provided by libc

#include <reent.h>
#include <stdlib.h>


void* _malloc_r(struct _reent* reent, size_t size)
{
    reent->_errno = 0;
    return malloc(size);
}


void _free_r(struct _reent* reent, void* p)
{
    reent->_errno = 0;
    free(p);
}


void* _calloc_r(struct _reent* reent, size_t size, size_t length)
{
    reent->_errno = 0;
    return calloc(size, length);
}


void* _realloc_r(struct _reent* reent, void* p, size_t size)
{
    reent->_errno = 0;
    return realloc(p, size);
}


#define _POSIX_THREADS

//#define LACKS_ERRNO_H 1
//#define LACKS_FCNTL_H 1
//#define LACKS_SCHED_H 1
//#define LACKS_STDLIB_H 1
//#define LACKS_STRING_H 1
//#define LACKS_STRINGS_H 1
//#define LACKS_SYS_MMAN_H 1
//#define LACKS_SYS_PARAM_H 1
//#define LACKS_SYS_TYPES_H 1
//#define LACKS_TIME_H 1
//#define LACKS_UNISTD_H 1

// Configuration
//#define NO_MALLOC_STATS 1
#define USE_LOCKS 1
//#define malloc_getpagesize MEMORY_PAGE_SIZE

// Fake mman.h implementation
// #define MAP_SHARED 1
// #define MAP_PRIVATE 2
// #define MAP_ANONYMOUS 4
// #define MAP_ANON MAP_ANONYMOUS
// #define MAP_FAILED ((void*)-1)
// #define PROT_NONE  0
// #define PROT_READ 1
// #define PROT_WRITE 2
// #define PROT_EXEC 4
#define HAVE_MORECORE 0
// #define MMAP_CLEARS 1

// Define our own locks
// #define MLOCK_T             Spinlock
// #define INITIAL_LOCK(mutex) (void)0
// #define DESTROY_LOCK(mutex) (void)0
// #define ACQUIRE_LOCK(mutex) ((mutex)->lock(), 0)
// #define RELEASE_LOCK(mutex) (mutex)->unlock()
// #define TRY_LOCK(mutex)     (mutex)->try_lock()

// static MLOCK_T malloc_global_mutex;


#include <dlmalloc/dlmalloc.inc>
