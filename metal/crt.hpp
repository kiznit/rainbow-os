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

#ifndef _RAINBOW_METAL_CRT_HPP
#define _RAINBOW_METAL_CRT_HPP

#include <stddef.h>
#include <stdint.h>

#undef assert

#if defined(NDEBUG)
#define assert(expression) ((void)(0))
#else
#define assert(expression) (__builtin_expect(!(expression), 0) ? __assert(#expression, __FILE__, __LINE__) : (void)0)
#endif


#define alloca(size) __builtin_alloca(size)


// C glue
extern "C"
{
    void __assert(const char* expression, const char* file, int line)  __attribute__ ((noreturn));

    void* memcpy(void*, const void*, size_t);
    void* memset(void*, int, size_t);
    int strcmp(const char* string1, const char* string2);
    size_t strlen(const char* string);

    // Heap memory
    void* calloc(size_t num, size_t size);
    void free(void* ptr);
    void* malloc(size_t size);
    void* memalign(size_t alignment, size_t size);
    void* realloc(void* ptr, size_t new_size);
}


// C++ glue
inline void* operator new(size_t, void* p)      { return p; }
inline void* operator new[](size_t, void* p)    { return p; }
inline void* operator new(size_t size)          { return ::malloc(size); }

inline void operator delete(void* p)            { ::free(p); }
inline void operator delete(void* p, size_t)    { ::free(p); }


#endif
