/*
    Copyright (c) 2024, Thierry Tremblay
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

#include <cstdlib>
#include <metal/exception.hpp>
#include <metal/helpers.hpp>
#include <metal/log.hpp>
#include <new>

#if defined(__MINGW32__)
#define PURECALL __cxa_pure_virtual
#else
#define PURECALL _purecall
#endif

extern "C" void PURECALL()
{
    MTL_LOG(Fatal) << (MTL_STRINGIZE(PURECALL) "()");
    abort();
}

extern "C" int atexit(void (*func)(void))
{
    return 0;
}

void* operator new(size_t size)
{
    auto memory = malloc(size);
    if (!memory)
        MTL_OUT_OF_MEMORY();
    return memory;
}

void operator delete(void* p)
{
    free(p);
}

void operator delete(void* p, size_t size)
{
    (void)size;
    free(p);
}

void* operator new[](size_t size)
{
    auto memory = malloc(size);
    if (!memory)
        MTL_OUT_OF_MEMORY();
    return memory;
}

void operator delete[](void* p)
{
    free(p);
}

void operator delete[](void* p, size_t size)
{
    (void)size;
    free(p);
}
