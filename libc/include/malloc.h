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

// TODO: non-standard header, very Unixy/Linuxy. Are we happy with this in our libc? move somewhere else?

#ifndef _LIBC_MALLOC_H
#define _LIBC_MALLOC_H

#ifdef __cplusplus
extern "C" {
#endif


struct mallinfo {
    int arena;      /* Non-mmapped space allocated (bytes) */
    int ordblks;    /* Number of free chunks */
    int smblks;     /* Number of free fastbin blocks */
    int hblks;      /* Number of mmapped regions */
    int hblkhd;     /* Space allocated in mmapped regions (bytes) */
    int usmblks;    /* Maximum total allocated space (bytes) */
    int fsmblks;    /* Space in freed fastbin blocks (bytes) */
    int uordblks;   /* Total allocated space (bytes) */
    int fordblks;   /* Total free space (bytes) */
    int keepcost;   /* Top-most, releasable space (bytes) */
};

void malloc_stats(void);
int malloc_trim(size_t pad);
size_t malloc_usable_size(void* ptr);
int mallopt(int param, int value);
void* memalign(size_t alignment, size_t size);
void* valloc(size_t size);
void* pvalloc(size_t size);

struct mallinfo mallinfo(void);


/*
    dlmalloc - Not standard at all! Keep?
*/
void* realloc_in_place(void* p, size_t n);
size_t malloc_footprint();
size_t malloc_max_footprint();
size_t malloc_footprint_limit();
size_t malloc_set_footprint_limit(size_t bytes);
void malloc_inspect_all(void(*handler)(void*, void *, size_t, void*), void* arg);
void** independent_calloc(size_t, size_t, void**);
void** independent_comalloc(size_t, size_t*, void**);

#ifdef __cplusplus
}
#endif

#endif
