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

// Note: the order of params matches the ABI used in the kernel (mregparm=3)
// syscall funtion     : ebx
// syscall parameters  : eax, edx, ecx, esi, edi, *ebp
// syscall return value: eax

#define __SYSENTER "call *0xEFFFF000\n"


static inline intptr_t __syscall0(intptr_t function)
{
    intptr_t result;

    asm volatile (
        __SYSENTER
        : "=a"(result)
        : "b"(function)
        : "memory"
    );

    return result;
}


static inline intptr_t __syscall1(intptr_t function, intptr_t arg1)
{
    intptr_t result;

    asm volatile (
        __SYSENTER
        : "=a"(result)
        : "b"(function),
          "a"(arg1)
        : "memory"
    );

    return result;
}


static inline intptr_t __syscall2(intptr_t function, intptr_t arg1, intptr_t arg2)
{
    intptr_t result;

    asm volatile (
        __SYSENTER
        : "=a"(result)
        : "b"(function),
          "a"(arg1),
          "d"(arg2)
        : "memory"
    );

    return result;
}


static inline intptr_t __syscall3(intptr_t function, intptr_t arg1, intptr_t arg2, intptr_t arg3)
{
    intptr_t result;

    asm volatile (
        __SYSENTER
        : "=a"(result)
        : "b"(function),
          "a"(arg1),
          "d"(arg2),
          "c"(arg3)
        : "memory"
    );

    return result;
}


static inline intptr_t __syscall4(intptr_t function, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4)
{
    intptr_t result;

    asm volatile (
        __SYSENTER
        : "=a"(result)
        : "b"(function),
          "a"(arg1),
          "d"(arg2),
          "c"(arg3),
          "S"(arg4)
        : "memory"
    );

    return result;
}


static inline intptr_t __syscall5(intptr_t function, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5)
{
    intptr_t result;

    asm volatile (
        __SYSENTER
        : "=a"(result)
        : "b"(function),
          "a"(arg1),
          "d"(arg2),
          "c"(arg3),
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

intptr_t __syscall6(intptr_t function, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6);


#undef __SYSENTER


#ifdef __cplusplus
}
#endif

#endif
