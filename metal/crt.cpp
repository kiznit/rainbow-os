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

#include "crt.hpp"
#include "log.hpp"


#if defined(__i386__) || defined(__x86_64__)
unsigned long __force_order; // See x86/cpu.hpp
#endif



extern "C" void _init()
{
    // Call global constructors
    extern void (* __init_array_start[])();
    extern void (* __init_array_end[])();

    for (auto init = __init_array_start; init != __init_array_end; ++init)
    {
        (*init)();
    }
}


extern "C" void __cxa_pure_virtual()
{
    Fatal("__cxa_pure_virtual()");
}


void __assert(const char* expression, const char* file, int line)
{
    Fatal("Assertion failed: %s at %s, line %d", expression, file, line);
}


extern "C" void* memcpy(void* dest, const void* src, size_t n)
{
    auto p = (char*)dest;
    auto q = (char*)src;

    while (n--)
        *p++ = *q++;

    return dest;
}


extern "C" void* memset(void* s, int c, size_t n)
{
    unsigned char* p = (unsigned char*)s;

    while (n--)
        *p++ = c;

    return s;
}


extern "C" int strcmp(const char* string1, const char* string2)
{
    while (*string1 && *string1 == *string2)
    {
        ++string1;
        ++string2;
    }

    return *(const unsigned char*)string1 - *(const unsigned char*)string2;
}


extern "C" size_t strlen(const char* string)
{
    size_t length = 0;
    while (*string++)
    {
        ++length;
    }

    return length;
}
