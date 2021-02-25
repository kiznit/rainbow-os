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

#ifdef __cplusplus
extern "C" {
#endif

// TODO: implement VDSO with ASLR
// TODO: use SYSCALL

// function / return value: rax
// Parameters: rdi, rsi, rdx, r10, r8, r9 (we can't use rcx for arg4 because SYSCALL will clobber it)
// note: syscall will clobber rcx and r11

#define __SYSCALL "syscall\n"


static inline intptr_t __syscall0(intptr_t function)
{
    intptr_t result;

    asm volatile (
        __SYSCALL
        : "=a"(result)
        : "a"(function)
        : "memory", "rcx", "r11"
    );

    return result;
}


static inline intptr_t __syscall1(intptr_t function, intptr_t arg1)
{
    intptr_t result;

    asm volatile (
        __SYSCALL
        : "=a"(result)
        : "a"(function),
          "D"(arg1)
        : "memory", "rcx", "r11"
    );

    return result;
}


static inline intptr_t __syscall2(intptr_t function, intptr_t arg1, intptr_t arg2)
{
    intptr_t result;

    asm volatile (
        __SYSCALL
        : "=a"(result)
        : "a"(function),
          "D"(arg1),
          "S"(arg2)
        : "memory", "rcx", "r11"
    );

    return result;
}


static inline intptr_t __syscall3(intptr_t function, intptr_t arg1, intptr_t arg2, intptr_t arg3)
{
    intptr_t result;

    asm volatile (
        __SYSCALL
        : "=a"(result)
        : "a"(function),
          "D"(arg1),
          "S"(arg2),
          "d"(arg3)
        : "memory", "rcx", "r11"
    );

    return result;
}


static inline intptr_t __syscall4(intptr_t function, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4)
{
    intptr_t result;

    register intptr_t r10 asm("r10") = arg4;

    asm volatile (
        __SYSCALL
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


static inline intptr_t __syscall5(intptr_t function, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5)
{
    intptr_t result;

    register intptr_t r10 asm("r10") = arg4;
    register intptr_t r8 asm("r8") = arg5;

    asm volatile (
        __SYSCALL
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


static inline intptr_t __syscall6(intptr_t function, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6)
{
    intptr_t result;

    register intptr_t r10 asm("r10") = arg4;
    register intptr_t r8 asm("r8") = arg5;
    register intptr_t r9 asm("r9") = arg6;

    asm volatile (
        __SYSCALL
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


#undef __SYSCALL


#ifdef __cplusplus
}
#endif

#endif
