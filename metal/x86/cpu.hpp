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

#ifndef _RAINBOW_METAL_X86_CPU_HPP
#define _RAINBOW_METAL_X86_CPU_HPP

#include <stddef.h>
#include <stdint.h>
#include "memory.hpp"


// EFLAGS
#define X86_EFLAGS_CF       0x00000001  // Carry
#define X86_EFLAGS_RESERVED 0x00000002  // Reserved and always 1
#define X86_EFLAGS_PF       0x00000004  // Parity
#define X86_EFLAGS_AF       0x00000010  // Auxiliary carry
#define X86_EFLAGS_ZF       0x00000040  // Zero
#define X86_EFLAGS_SF       0x00000080  // Sign
#define X86_EFLAGS_TF       0x00000100  // Trap
#define X86_EFLAGS_IF       0x00000200  // Interrupt enable
#define X86_EFLAGS_DF       0x00000400  // Direction
#define X86_EFLAGS_OF       0x00000800  // Overflow
#define X86_EFLAGS_IOPL     0x00003000  // Input/Output Priviledge Level
#define X86_EFLAGS_NT       0x00004000  // Nested Task
#define X86_EFLAGS_RF       0x00010000  // Resume
#define X86_EFLAGS_VM       0x00020000  // Virtual 8086 Mode

// CR0
#define X86_CR0_PG (1 << 31)

// CR4
#define X86_CR4_VME         (1 << 0)    // Virtual 8086 Mode Extensions
#define X86_CR4_PVI         (1 << 1)    // Protected-mode Virtual Interrupts
#define X86_CR4_TSD         (1 << 2)    // Time Stamp Disable
#define X86_CR4_DE          (1 << 3)    // Debugging Extensions
#define X86_CR4_PSE         (1 << 4)    // Page Size Extension (if set, pages are 4MB)
#define X86_CR4_PAE         (1 << 5)    // Physical Address Extension (36 bits physical addresses)
#define X86_CR4_MCE         (1 << 6)    // Machine Check Exceptions enable
#define X86_CR4_PGE         (1 << 7)    // Page Global Enabled (if set, PTE may be shared between address spaces)
#define X86_CR4_PCE         (1 << 8)    // Performance-Monitoring Counter enable
#define X86_CR4_OSFXSR      (1 << 9)    // SSE enable
#define X86_CR4_OSXMMEXCPT  (1 << 10)   // SSE Exceptions enable
#define X86_CR4_UMIP        (1 << 11)   // User-Mode Instruction Prevention
#define X86_CR4_LA57        (1 << 12)   // 5-level paging enable
#define X86_CR4_VMXE        (1 << 13)   // Virtual Machine Extensions Enable
#define X86_CR4_SMXE        (1 << 14)   // Safer Mode Extensions Enable
#define X86_CR4_FSGSBASE    (1 << 16)   // Enables RDFSBASE, RDGSBASE, WRFSBASE, WRGSBASE instructions
#define X86_CR4_PCIDE       (1 << 17)   // Process-Context Identifiers enable
#define X86_CR4_OSXSAVE     (1 << 18)   // XSAVE and Processor Extended States enable
#define X86_CR4_SMEP        (1 << 20)   // Supervisor Mode Execution Protection Enable
#define X86_CR4_SMAP        (1 << 21)   // Supervisor Mode Access Prevention Enable
#define X86_CR4_PKE         (1 << 22)   // Protection Key Enable


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


static inline uintptr_t x86_get_cr2()
{
    uintptr_t value;
    asm ("mov %%cr2, %0" : "=r"(value), "=m" (__force_order));
    return value;
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


// ia32 MSRs
#define MSR_SYSENTER_CS     0x00000174
#define MSR_SYSENTER_ESP    0x00000175
#define MSR_SYSENTER_EIP    0x00000176


// x86-64 specific MSRs
#define MSR_EFER            0xc0000080 // extended feature register
#define MSR_STAR            0xc0000081 // Legacy mode SYSCALL target
#define MSR_LSTAR           0xc0000082 // Long mode SYSCALL target
#define MSR_CSTAR           0xc0000083 // Compat mode SYSCALL target
#define MSR_FMASK           0xc0000084 // EFLAGS mask for SYSCALL
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



// Invalidate page tables for the specified address
static inline void x86_invlpg(const void* virtualAddress)
{
    asm volatile ("invlpg (%0)" : : "r"(virtualAddress) : "memory");
}


// GDT / Segment Descriptors
struct GdtDescriptor
{
    uint16_t limit;
    uint16_t base;
    uint16_t flags1;
    uint16_t flags2;

    // Initialize a 32 bits kernel data descriptor with the specified base and size
    void SetKernelData32(uint32_t base, uint32_t size);
};


struct GdtPtr
{
    uint16_t size;
    void* address;
} __attribute__((packed));


static inline void x86_lgdt(const GdtPtr& gdt)
{
    asm volatile ("lgdt %0" : : "m" (gdt) );
}


// IDT / Interrupt descriptors
struct IdtDescriptor
{
    uint16_t offset_low;
    uint16_t selector;
    uint16_t flags;
    uint16_t offset_mid;
#if defined(__x86_64__)
    uint32_t offset_high;
    uint32_t reserved;
#endif
};


struct IdtPtr
{
    uint16_t size;
    void* address;
} __attribute__((packed));


static inline void x86_lidt(const IdtPtr& idt)
{
    asm volatile ("lidt %0" : : "m" (idt) );
}


// There is a hardware constraint where we have to make sure that a TSS doesn't cross
// page boundary. If that happen, invalid data might be loaded during a task switch.
//
// TSS is hard, see http://www.os2museum.com/wp/the-history-of-a-security-hole/

struct Tss32
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


struct Tss64
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


// Sanity checks
static_assert(sizeof(Tss32) == 0x68, "Tss32 has unexpected size");
static_assert(sizeof(Tss64) == 0x68, "Tss64 has unexpected size");

// FXSAVE storage
struct FpuState
{
    uint16_t fcw;
    uint16_t fsw;
    uint8_t  ftw;
    uint8_t  reserved1;
    uint16_t fop;
    uint32_t fip;
    uint16_t fcs;
    uint16_t rsvd;

    uint32_t fdp;
    uint16_t fds;
    uint16_t reserved2;
    uint32_t mxcsr;
    uint32_t mxcsr_mask;

    uint8_t mm0[16];
    uint8_t mm1[16];
    uint8_t mm2[16];
    uint8_t mm3[16];
    uint8_t mm4[16];
    uint8_t mm5[16];
    uint8_t mm6[16];
    uint8_t mm7[16];

    uint8_t xmm0[16];
    uint8_t xmm1[16];
    uint8_t xmm2[16];
    uint8_t xmm3[16];
    uint8_t xmm4[16];
    uint8_t xmm5[16];
    uint8_t xmm6[16];
    uint8_t xmm7[16];

#if defined(__i386__)
    uint8_t reserved3[8*16];
#elif defined(__x86_64__)
    uint8_t xmm8[16];
    uint8_t xmm9[16];
    uint8_t xmm10[16];
    uint8_t xmm11[16];
    uint8_t xmm12[16];
    uint8_t xmm13[16];
    uint8_t xmm14[16];
    uint8_t xmm15[16];
#endif

    uint8_t reserved4[3*16];
    uint8_t available[3*16];

} __attribute__((packed, aligned(16)));


// Sanity checks
static_assert(sizeof(FpuState) == 512, "FpuState has unexpected size");


#if defined(__i386__)

static inline void x86_fxsave(FpuState* state)
{
    asm volatile ("fxsave %0" : "=m"(*state));
}

static inline void x86_fxrstor(FpuState* state)
{
    asm volatile ("fxrstor %0" : : "m"(*state));
}

#elif defined(__x86_64__)

static inline void x86_fxsave64(FpuState* state)
{
    asm volatile ("fxsave64 %0" : "=m"(*state));
}

static inline void x86_fxrstor64(FpuState* state)
{
    asm volatile ("fxrstor64 %0" : : "m"(*state));
}

#endif



#endif

