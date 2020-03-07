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

#ifndef _RAINBOW_KERNEL_IA32_SYSCALL_HPP
#define _RAINBOW_KERNEL_IA32_SYSCALL_HPP

#include <stdint.h>


struct SysCallParams
{
    uint32_t cr2;

    uint16_t ds;
    uint16_t es;
    uint16_t fs;
    uint16_t gs;

    union
    {
        uint32_t function;  // eax
        uint32_t result;    // eax
    };
    uint32_t arg1;      // ebx
    uint32_t arg2;      // ecx
    uint32_t arg3;      // edx
    uint32_t arg4;      // esi
    uint32_t arg5;      // edi
    uint32_t arg6;      // ebp

    uint32_t interrupt;
    uint32_t error;
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;

    // These are only saved/restored when crossing priviledge levels
    uint32_t esp;
    uint32_t ss;
};


#endif
