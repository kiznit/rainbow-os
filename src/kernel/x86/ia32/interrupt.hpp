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

#ifndef _RAINBOW_KERNEL_IA32_INTERRUPT_HPP
#define _RAINBOW_KERNEL_IA32_INTERRUPT_HPP

#include <stdint.h>


struct InterruptContext
{
    // Note: keep syscall arguments on top. We invoke handlers directly
    // and the stack needs to be setup properly with the arguments in the
    // right order.

    uint32_t ebx;   // Syscall arg 1
    uint32_t ecx;   // Syscall arg 2
    uint32_t edx;   // Syscall arg 3
    uint32_t esi;   // Syscall arg 4
    uint32_t edi;   // Syscall arg 5
    uint32_t ebp;   // Syscall user stack - arg 6 at %ebp(0)
    uint32_t eax;   // Syscall function number and return value

    uint16_t ds;
    uint16_t ds_h;
    uint16_t es;
    uint16_t es_h;
    uint16_t fs;
    uint16_t fs_h;
    uint16_t gs;
    uint16_t gs_h;

    union
    {
        uint32_t error;
        uint32_t interrupt;
        uint32_t syscall;
    };

    // iret frame - defined by architecture
    uint32_t eip;
    uint16_t cs;
    uint16_t cs_h;
    uint32_t eflags;
    // These are only saved/restored when crossing priviledge levels
    uint32_t esp;
    uint16_t ss;
    uint16_t ss_h;
} __attribute__((packed));


#endif
