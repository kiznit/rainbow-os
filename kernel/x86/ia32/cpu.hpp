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

#ifndef _RAINBOW_KERNEL_IA32_CPU_HPP
#define _RAINBOW_KERNEL_IA32_CPU_HPP

#include <metal/x86/cpu.hpp>

class Task;


// PerCpu is used to hold per-cpu data that doesn't need to be accessed from
// other cpus. PerCpu objects are accessible to the running CPU by using the
// GS segment. See the macros below to read/write data to the PerCpu object.
struct PerCpu
{
    GdtDescriptor*  gdt;    // GDT
    Tss32*          tss;    // TSS
    Task*           task;   // Currently executing task

    // There is a hardware constraint where we have to make sure that a TSS doesn't cross
    // page boundary. If that happen, invalid data might be loaded during a task switch.
    // Aligning the TSS to 128 bytes is enough to ensure that (128 > sizeof(Tss)).
    // TODO: is having the TSS inside PerCpu a leaking concern (meltdown/spectre)?
    Tss32           tss32 __attribute__((aligned(128)));
};


// Read data from the PerCpu object.
#define cpu_get_data(fieldName) ({ \
    typeof(PerCpu::fieldName) result; \
    asm ("mov %%gs:%1, %0" : "=r"(result) : "m"(*(typeof(PerCpu::fieldName)*)offsetof(PerCpu, fieldName))); \
    result; \
})

// Write data to the PerCpu object.
#define cpu_set_data(fieldName, value) ({ \
    asm ("mov %0, %%gs:%1" : : "r"(value), "m"(*(typeof(PerCpu::fieldName)*)offsetof(PerCpu, fieldName))); \
})


#endif
