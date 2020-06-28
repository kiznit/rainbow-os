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

#ifndef _RAINBOW_ARCH_X86_64_SYSCALL_H
#define _RAINBOW_ARCH_X86_64_SYSCALL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// TODO: implement VDSO with ASLR
// TODO: use SYSCALL

// function / return value: rax
// Parameters: rdi, rsi, rdx, r10, r8, r9 (we can't use rcx for arg4 because SYSCALL will clobber it)
// note: syscall will clobber rcx and r11

#define SYSCALL "syscall\n"


static inline int64_t syscall0(int64_t function)
{
    int64_t result;

    asm volatile (
        SYSCALL
        : "=a"(result)
        : "a"(function)
        : "memory", "rcx", "r11"
    );

    return result;
}


static inline int64_t syscall1(int64_t function, int64_t arg1)
{
    int64_t result;

    asm volatile (
        SYSCALL
        : "=a"(result)
        : "a"(function),
          "D"(arg1)
        : "memory", "rcx", "r11"
    );

    return result;
}


static inline int64_t syscall2(int64_t function, int64_t arg1, int64_t arg2)
{
    int64_t result;

    asm volatile (
        SYSCALL
        : "=a"(result)
        : "a"(function),
          "D"(arg1),
          "S"(arg2)
        : "memory", "rcx", "r11"
    );

    return result;
}


static inline int64_t syscall3(int64_t function, int64_t arg1, int64_t arg2, int64_t arg3)
{
    int64_t result;

    asm volatile (
        SYSCALL
        : "=a"(result)
        : "a"(function),
          "D"(arg1),
          "S"(arg2),
          "d"(arg3)
        : "memory", "rcx", "r11"
    );

    return result;
}


static inline int64_t syscall4(int64_t function, int64_t arg1, int64_t arg2, int64_t arg3, int64_t arg4)
{
    int64_t result;

    register int64_t r10 asm("r10") = arg4;

    asm volatile (
        SYSCALL
        : "=a"(result)
        : "a"(function),
          "D"(arg1),
          "S"(arg2),
          "d"(arg3),
          "r"(r10)
        : "memory", "rcx", "r11"
    );

    return result;
}


static inline int64_t syscall5(int64_t function, int64_t arg1, int64_t arg2, int64_t arg3, int64_t arg4, int64_t arg5)
{
    int64_t result;

    register int64_t r10 asm("r10") = arg4;
    register int64_t r8 asm("r8") = arg5;

    asm volatile (
        SYSCALL
        : "=a"(result)
        : "a"(function),
          "D"(arg1),
          "S"(arg2),
          "d"(arg3),
          "r"(r10),
          "r"(r8)
        : "memory", "rcx", "r11"
    );

    return result;
}


static inline int64_t syscall6(int64_t function, int64_t arg1, int64_t arg2, int64_t arg3, int64_t arg4, int64_t arg5, int64_t arg6)
{
    int64_t result;

    register int64_t r10 asm("r10") = arg4;
    register int64_t r8 asm("r8") = arg5;
    register int64_t r9 asm("r9") = arg6;

    asm volatile (
        SYSCALL
        : "=a"(result)
        : "a"(function),
          "D"(arg1),
          "S"(arg2),
          "d"(arg3),
          "r"(r10),
          "r"(r8),
          "r"(r9)
        : "memory", "rcx", "r11"
    );

    return result;
}


#undef SYSCALL


#ifdef __cplusplus
}
#endif

#endif
