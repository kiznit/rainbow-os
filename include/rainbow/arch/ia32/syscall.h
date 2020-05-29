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

#ifndef _RAINBOW_ARCH_IA32_SYSCALL_H
#define _RAINBOW_ARCH_IA32_SYSCALL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


// TODO: implement proper VDSO with ASLR

// function / return value: eax
// parameters: ebx, ecx, edx, esi, edi

#define SYSENTER "call *0xEFFFF000\n"


static inline int32_t syscall0(int32_t function)
{
    int32_t result;

    asm volatile (
        SYSENTER
        : "=a"(result)
        : "a"(function)
        : "memory"
    );

    return result;
}


static inline int32_t syscall1(int32_t function, int32_t arg1)
{
    int32_t result;

    asm volatile (
        SYSENTER
        : "=a"(result)
        : "a"(function),
          "b"(arg1)
        : "memory"
    );

    return result;
}


static inline int32_t syscall2(int32_t function, int32_t arg1, int32_t arg2)
{
    int32_t result;

    asm volatile (
        SYSENTER
        : "=a"(result)
        : "a"(function),
          "b"(arg1),
          "c"(arg2)
        : "memory"
    );

    return result;
}


static inline int32_t syscall3(int32_t function, int32_t arg1, int32_t arg2, int32_t arg3)
{
    int32_t result;

    asm volatile (
        SYSENTER
        : "=a"(result)
        : "a"(function),
          "b"(arg1),
          "c"(arg2),
          "d"(arg3)
        : "memory"
    );

    return result;
}


static inline int32_t syscall4(int32_t function, int32_t arg1, int32_t arg2, int32_t arg3, int32_t arg4)
{
    int32_t result;

    asm volatile (
        SYSENTER
        : "=a"(result)
        : "a"(function),
          "b"(arg1),
          "c"(arg2),
          "d"(arg3),
          "S"(arg4)
        : "memory"
    );

    return result;
}


static inline int32_t syscall5(int32_t function, int32_t arg1, int32_t arg2, int32_t arg3, int32_t arg4, int32_t arg5)
{
    int32_t result;

    asm volatile (
        SYSENTER
        : "=a"(result)
        : "a"(function),
          "b"(arg1),
          "c"(arg2),
          "d"(arg3),
          "S"(arg4),
          "D"(arg5)
        : "memory"
    );

    return result;
}


#undef SYSENTER


#ifdef __cplusplus
}
#endif

#endif
