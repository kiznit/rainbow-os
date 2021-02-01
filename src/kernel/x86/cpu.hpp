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

#ifndef _RAINBOW_KERNEL_X86_CPU_HPP
#define _RAINBOW_KERNEL_X86_CPU_HPP

#include <vector>
#include <metal/x86/cpu.hpp>

class Task;

#if defined(__i386__)
using Tss = Tss32;
#elif defined(__x86_64__)
extern "C" void syscall_entry();
using Tss = Tss64;
#endif


// TODO: allocate using vmm_allocate_pages() (?)
// TODO: make non-copyable / (non-moveable?)
class Cpu
{
public:

    void* operator new(size_t size);
    void operator delete(void* p);

    Cpu(int id, int apicId, bool enabled, bool bootstrap);

    const int       id;             // Processor id (>= 0)
    const int       apicId;         // Local APIC id (>= 0)
    const bool      enabled;        // Processor is enabled, otherwise it needs to be brought online
    const bool      bootstrap;      // Is this the boostrap processor (BSP)?

    GdtDescriptor*  gdt;            // GDT
    Tss*            tss;            // TSS
    Task*           task;           // Currently executing task

#if defined(__x86_64__)
    uint64_t        userStack;      // Holds user rsp temporarily during syscall to setup kernel stack
    uint64_t        kernelStack;    // Holds kernel rsp for syscall
#endif

    // There is a hardware constraint where we have to make sure that a TSS doesn't cross
    // page boundary. If that happen, invalid data might be loaded during a task switch.
    // Aligning the TSS to 128 bytes is enough to ensure that (128 > sizeof(Tss)).
    // TODO: is having the TSS her a leaking concern (meltdown/spectre)?
    Tss             tss_ __attribute__((aligned(128)));

    // Initialize the hardware CPU
    void Initialize();

private:
    void InitGdt();
    void InitTss();
};


extern std::vector<Cpu*> g_cpus;


#if defined(__i386__)
#define CPUDATA_SEGMENT "fs"
#elif defined(__x86_64__)
#define CPUDATA_SEGMENT "gs"
#endif

// Read data from the Cpu object using the CPUDATA_SEGMENT segment.
#define cpu_get_data(fieldName) ({ \
    std::remove_const<typeof(Cpu::fieldName)>::type result; \
    asm ("mov %%" CPUDATA_SEGMENT ":%1, %0" : "=r"(result) : "m"(*(typeof(Cpu::fieldName)*)offsetof(Cpu, fieldName))); \
    result; \
})

// Write data to the Cpu object using the CPUDATA_SEGMENT segment.
#define cpu_set_data(fieldName, value) ({ \
    asm ("mov %0, %%" CPUDATA_SEGMENT ":%1" : : "r"(value), "m"(*(typeof(Cpu::fieldName)*)offsetof(Cpu, fieldName))); \
})


#endif
