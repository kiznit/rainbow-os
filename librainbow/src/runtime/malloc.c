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

/*
    This file provides the malloc implementation.

    Currently we compile newlib without malloc() support. This was done because
    I wanted to use mmap() instead of sbrk(), especially in the kernel.

    In the future we will want to replace this with a SMP friendly malloc implementation.

    TODO: We also want malloc() to be part of libc, not part of librainbow!!!
*/

#include <reent.h>
#include <stdlib.h>

// GCC is smart enough to optimize malloc() + memset() into calloc(). This results
// in an infinite loop when calling calloc() because it is basically implemented
// by calling malloc() + memset(). This will disable the optimization.
#pragma GCC optimize "no-optimize-strlen"


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


// Configuration
#define USE_LOCKS 1         // Will use internal spinlocks
#define HAVE_MORECORE 0     // Disable sbrk()



#include <dlmalloc/dlmalloc.inc>
