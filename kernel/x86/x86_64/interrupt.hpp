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

#ifndef _RAINBOW_KERNEL_X86_64_INTERRUPT_HPP
#define _RAINBOW_KERNEL_X86_64_INTERRUPT_HPP

#include <stdint.h>


struct InterruptContext
{
    uint64_t rax;   // Syscall function number and return value
    uint64_t rbx;
    uint64_t rcx;   // Syscall user rip
    uint64_t rdx;   // Syscall arg3
    uint64_t rsi;   // Syscall arg2
    uint64_t rdi;   // Syscall arg1
    uint64_t rbp;
    uint64_t r8;    // Syscall arg5
    uint64_t r9;    // Syscall arg6
    uint64_t r10;   // Syscall arg4
    uint64_t r11;   // Syscall user rflags
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;

    union
    {
        uint64_t error;
        uint64_t interrupt;
        uint64_t syscall;
    };

    // iret frame - defined by architecture
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    // These are always valid (different behaviour than 32 bits mode)
    uint64_t rsp;
    uint64_t ss;
} __attribute__((packed));


#endif
