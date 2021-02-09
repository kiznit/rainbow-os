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

#ifdef __cplusplus
extern "C" {
#endif


// TODO: implement proper VDSO with ASLR

// function / return value: eax
// parameters: ebx, ecx, edx, esi, edi, *ebp

#define __SYSENTER "call *0xEFFFF000\n"


static inline long __syscall0(long function)
{
    long result;

    asm volatile (
        __SYSENTER
        : "=a"(result)
        : "a"(function)
        : "memory"
    );

    return result;
}


static inline long __syscall1(long function, long arg1)
{
    long result;

    asm volatile (
        __SYSENTER
        : "=a"(result)
        : "a"(function),
          "b"(arg1)
        : "memory"
    );

    return result;
}


static inline long __syscall2(long function, long arg1, long arg2)
{
    long result;

    asm volatile (
        __SYSENTER
        : "=a"(result)
        : "a"(function),
          "b"(arg1),
          "c"(arg2)
        : "memory"
    );

    return result;
}


static inline long __syscall3(long function, long arg1, long arg2, long arg3)
{
    long result;

    asm volatile (
        __SYSENTER
        : "=a"(result)
        : "a"(function),
          "b"(arg1),
          "c"(arg2),
          "d"(arg3)
        : "memory"
    );

    return result;
}


static inline long __syscall4(long function, long arg1, long arg2, long arg3, long arg4)
{
    long result;

    asm volatile (
        __SYSENTER
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


static inline long __syscall5(long function, long arg1, long arg2, long arg3, long arg4, long arg5)
{
    long result;

    asm volatile (
        __SYSENTER
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


/*
    syscall6 doesn't work well with inline assembly. Some compilers are happy with it, others refuse to use ebp for the last paramter.
    so for now, we will have libc implement it as a non-inline function.
*/

long __syscall6(long function, long arg1, long arg2, long arg3, long arg4, long arg5, long arg6);


#undef __SYSENTER


#ifdef __cplusplus
}
#endif

#endif
