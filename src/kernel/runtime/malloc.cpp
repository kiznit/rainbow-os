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

#include <kernel/spinlock.hpp>
#include <kernel/vmm.hpp>


// GCC is smart enough to optimize malloc() + memset() into calloc(). This results
// in an infinite loop when calling calloc() because it is basically implemented
// by calling malloc() + memset(). This will disable the optimization.
#pragma GCC optimize "no-optimize-strlen"

// Configuration
#define HAVE_MMAP 0
#define LACKS_TIME_H 1

#define NO_MALLOC_STATS 1
#define USE_LOCKS 2
#define malloc_getpagesize MEMORY_PAGE_SIZE

// Define our own locks
// Careful as these will be used "early" when calling global constructors.
// This means that the locks must not try to access globals / things that
// are not yet initialized (so a RecursiveSpinlock would not work).
// Technically global Spinlock objects are not initialized yet, but that's
// probably fine as they should default to unlocked (0).
#define MLOCK_T             Spinlock
#define INITIAL_LOCK(mutex) (void)0
#define DESTROY_LOCK(mutex) (void)0
#define ACQUIRE_LOCK(mutex) ((mutex)->lock(), 0)
#define RELEASE_LOCK(mutex) (mutex)->unlock()
#define TRY_LOCK(mutex)     (mutex)->try_lock()

static MLOCK_T malloc_global_mutex;

#define MORECORE vmm_sbrk


#include <dlmalloc/dlmalloc.inc>
