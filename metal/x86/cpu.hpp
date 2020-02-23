/*
    Copyright (c) 2018, Thierry Tremblay
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

#ifndef _RAINBOW_METAL_X86_CPU_HPP
#define _RAINBOW_METAL_X86_CPU_HPP

#include <stdint.h>
#include <metal/x86/cpu.hpp>


// EFLAGS
#define X86_EFLAGS_IF 0x00000200

// CR0
#define X86_CR0_PG (1 << 31)

// CR4
#define X86_CR4_PSE (1 << 4)    // Page Size Extension (if set, pages are 4MB)
#define X86_CR4_PAE (1 << 5)    // Physical Address Extension (36 bits physical addresses)
#define X86_CR4_PGE (1 << 7)    // Page Global Enabled (if set, PTE may be shared between address spaces)



/*
 * Control registers
 *
 * Volatile isn't enough to prevent the compiler from reordering the
 * read/write functions for the control registers and messing everything up.
 * A memory clobber would solve the problem, but would prevent reordering of
 * all loads/stores around it, which can hurt performance. The solution is to
 * use a variable and mimic reads and writes to it to enforce serialization
 */

extern unsigned long __force_order;


static inline uintptr_t x86_get_cr0()
{
    uintptr_t value;
    asm ("mov %%cr0, %0" : "=r"(value), "=m" (__force_order));
    return value;
}


static inline void x86_set_cr0(uintptr_t value)
{
    asm volatile ("mov %0, %%cr0" : : "r"(value), "m" (__force_order));
}


static inline uintptr_t x86_get_cr3()
{
    uintptr_t physicalAddress;
    asm ("mov %%cr3, %0" : "=r"(physicalAddress), "=m" (__force_order));
    return physicalAddress;
}


static inline void x86_set_cr3(uintptr_t physicalAddress)
{
    asm volatile ("mov %0, %%cr3" : : "r"(physicalAddress), "m" (__force_order));
}


static inline uintptr_t x86_get_cr4()
{
    uintptr_t value;
    asm ("mov %%cr4, %0" : "=r"(value), "=m" (__force_order));
    return value;
}


static inline void x86_set_cr4(uintptr_t value)
{
    asm volatile ("mov %0, %%cr4" : : "r"(value), "m" (__force_order));
}


static inline void x86_load_task_register(uint16_t selector)
{
    asm volatile ("ltr %0" : : "r"(selector));
}


// x86-64 specific MSRs
#define MSR_EFER            0xc0000080 // extended feature register
#define MSR_STAR            0xc0000081 // legacy mode SYSCALL target
#define MSR_LSTAR           0xc0000082 // long mode SYSCALL target
#define MSR_CSTAR           0xc0000083 // compat mode SYSCALL target
#define MSR_SYSCALL_MASK    0xc0000084 // EFLAGS mask for syscall
#define MSR_FS_BASE         0xc0000100 // 64bit FS base
#define MSR_GS_BASE         0xc0000101 // 64bit GS base
#define MSR_KERNEL_GS_BASE  0xc0000102 // SwapGS GS shadow
#define MSR_TSC_AUX         0xc0000103 // Auxiliary TSC


// MSR_EFER bits
#define EFER_SCE    (1 << 0)    // SYSCALL / SYSRET
#define EFER_LME    (1 << 8)    // Long mode enable
#define EFER_LMA    (1 << 10)   // Long mode active (read-only)
#define EFER_NX     (1 << 11)   // No execute enable
#define EFER_SVME   (1 << 12)   // Enable virtualization
#define EFER_LMSLE  (1 << 13)   // Long mode segment limit enable
#define EFER_FFXSR  (1 << 14)   // Enable fast FXSAVE/FXRSTOR


static inline uint64_t x86_read_msr(unsigned int msr)
{
#if defined(__i386__)
    uint64_t value;
    asm volatile ("rdmsr" : "=A" (value) : "c" (msr));
    return value;
#elif defined(__x86_64__)
    uint32_t low, high;
    asm volatile ("rdmsr" : "=a" (low), "=d" (high) : "c" (msr));
    return (uint64_t(high) << 32) | low;
#endif
}


static inline void x86_write_msr(unsigned int msr, uint64_t value)
{
    const uint32_t low = value & 0xFFFFFFFF;
    const uint32_t high = value >> 32;

    asm volatile ("wrmsr" : : "c" (msr), "a"(low), "d" (high) : "memory");
}


#if defined(__i386__)

struct Tss
{
   uint32_t link;       // Link to previous TSS when using hardware task switching (we are not)
   uint32_t esp0;       // esp when entering ring 0
   uint32_t ss0;        // ss when entering ring 0
   uint32_t esp1;       // Everything from here to the end is unused...
   uint32_t ss1;
   uint32_t esp2;
   uint32_t ss2;
   uint32_t cr3;
   uint32_t eip;
   uint32_t eflags;
   uint32_t eax;
   uint32_t ecx;
   uint32_t edx;
   uint32_t ebx;
   uint32_t esp;
   uint32_t ebp;
   uint32_t esi;
   uint32_t edi;
   uint32_t es;
   uint32_t cs;
   uint32_t ss;
   uint32_t ds;
   uint32_t fs;
   uint32_t gs;
   uint32_t ldt;
   uint16_t reserved;
   uint16_t iomap;
} __attribute__((packed));

#elif defined(__x86_64__)

struct Tss
{
    uint32_t reserved0;
    uint64_t rsp0;      // rsp when entering ring 0
    uint64_t rsp1;      // rsp when entering ring 1
    uint64_t rsp2;      // rsp when entering ring 2
    uint64_t reserved1;
    // The next 7 entries are the "Interrupt stack Table"
    // Here we can defined pointers to stack to handle interrupts.
    // Which one to use is defined in the Interrupt Descriptor Table.
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap;

} __attribute__((packed));

#endif

static_assert(sizeof(Tss) == 0x68, "Tss has unexpected size");


#endif

