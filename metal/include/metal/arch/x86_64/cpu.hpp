/*
    Copyright (c) 2023, Thierry Tremblay
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

#pragma once

#include <cstdint>

namespace mtl
{
    // EFLAGS
    constexpr auto EFLAGS_CF = 0x00000001;       // Carry
    constexpr auto EFLAGS_RESERVED = 0x00000002; // Reserved and always 1
    constexpr auto EFLAGS_PF = 0x00000004;       // Parity
    constexpr auto EFLAGS_AF = 0x00000010;       // Auxiliary carry
    constexpr auto EFLAGS_ZF = 0x00000040;       // Zero
    constexpr auto EFLAGS_SF = 0x00000080;       // Sign
    constexpr auto EFLAGS_TF = 0x00000100;       // Trap
    constexpr auto EFLAGS_IF = 0x00000200;       // Interrupt enable
    constexpr auto EFLAGS_DF = 0x00000400;       // Direction
    constexpr auto EFLAGS_OF = 0x00000800;       // Overflow
    constexpr auto EFLAGS_IOPL = 0x00003000;     // Input/Output Priviledge Level
    constexpr auto EFLAGS_NT = 0x00004000;       // Nested Task
    constexpr auto EFLAGS_RF = 0x00010000;       // Resume
    constexpr auto EFLAGS_VM = 0x00020000;       // Virtual 8086 Mode

    /*
     * Control registers
     */

    constexpr auto CR0_PG = 1 << 31;

    // CR4
    constexpr auto CR4_VME = 1 << 0;         // Virtual 8086 Mode Extensions
    constexpr auto CR4_PVI = 1 << 1;         // Protected-mode Virtual Interrupts
    constexpr auto CR4_TSD = 1 << 2;         // Time Stamp Disable
    constexpr auto CR4_DE = 1 << 3;          // Debugging Extensions
    constexpr auto CR4_PSE = 1 << 4;         // Page Size Extension (if set, pages are 4MB)
    constexpr auto CR4_PAE = 1 << 5;         // Physical Address Extension (36 bits physical addresses)
    constexpr auto CR4_MCE = 1 << 6;         // Machine Check Exceptions enable
    constexpr auto CR4_PGE = 1 << 7;         // Page Global Enabled (if set, PTE may be shared between address spaces)
    constexpr auto CR4_PCE = 1 << 8;         // Performance-Monitoring Counter enable
    constexpr auto CR4_OSFXSR = 1 << 9;      // SSE enable
    constexpr auto CR4_OSXMMEXCPT = 1 << 10; // SSE Exceptions enable
    constexpr auto CR4_UMIP = 1 << 11;       // User-Mode Instruction Prevention
    constexpr auto CR4_LA57 = 1 << 12;       // 5-level paging enable
    constexpr auto CR4_VMXE = 1 << 13;       // Virtual Machine Extensions Enable
    constexpr auto CR4_SMXE = 1 << 14;       // Safer Mode Extensions Enable
    constexpr auto CR4_FSGSBASE = 1 << 16;   // Enables RDFSBASE, RDGSBASE, WRFSBASE, WRGSBASE instructions
    constexpr auto CR4_PCIDE = 1 << 17;      // Process-Context Identifiers enable
    constexpr auto CR4_OSXSAVE = 1 << 18;    // XSAVE and Processor Extended States enable
    constexpr auto CR4_SMEP = 1 << 20;       // Supervisor Mode Execution Protection Enable
    constexpr auto CR4_SMAP = 1 << 21;       // Supervisor Mode Access Prevention Enable
    constexpr auto CR4_PKE = 1 << 22;        // Protection Key Enable

    /*
     *
     * Volatile isn't enough to prevent the compiler from reordering the
     * read/write functions for the control registers and messing everything up.
     * A memory clobber would solve the problem, but would prevent reordering of
     * all loads/stores around it, which can hurt performance. The solution is to
     * use a variable and mimic reads and writes to it to enforce serialization
     */

    extern unsigned int __force_order;

    static inline uintptr_t Read_CR0()
    {
        uintptr_t value;
        asm("mov %%cr0, %0" : "=r"(value), "=m"(__force_order));
        return value;
    }

    static inline void Write_CR0(uintptr_t value)
    {
        asm volatile("mov %0, %%cr0" : : "r"(value), "m"(__force_order));
    }

    static inline uintptr_t Read_CR2()
    {
        uintptr_t value;
        asm("mov %%cr2, %0" : "=r"(value), "=m"(__force_order));
        return value;
    }

    static inline uintptr_t Read_CR3()
    {
        uintptr_t physicalAddress;
        asm("mov %%cr3, %0" : "=r"(physicalAddress), "=m"(__force_order));
        return physicalAddress;
    }

    static inline void Write_CR3(uintptr_t physicalAddress)
    {
        asm volatile("mov %0, %%cr3" : : "r"(physicalAddress), "m"(__force_order));
    }

    static inline uintptr_t Read_CR4()
    {
        uintptr_t value;
        asm("mov %%cr4, %0" : "=r"(value), "=m"(__force_order));
        return value;
    }

    static inline void Write_CR4(uintptr_t value)
    {
        asm volatile("mov %0, %%cr4" : : "r"(value), "m"(__force_order));
    }

    /*
     * Model Specific Registers (MSR)
     */

    enum class Msr : unsigned int
    {
        IA32_MTRRCAP = 0x000000FE,

        IA32_SYSENTER_CS = 0x00000174,
        IA32_SYSENTER_ESP = 0x00000175,
        IA32_SYSENTER_EIP = 0x00000176,

        // Variable Range MTRRs
        IA32_MTRR_PHYSBASE0 = 0x00000200,
        IA32_MTRR_PHYSMASK0 = 0x00000201,
        IA32_MTRR_PHYSBASE1 = 0x00000202,
        IA32_MTRR_PHYSMASK1 = 0x00000203,
        IA32_MTRR_PHYSBASE2 = 0x00000204,
        IA32_MTRR_PHYSMASK2 = 0x00000205,
        IA32_MTRR_PHYSBASE3 = 0x00000206,
        IA32_MTRR_PHYSMASK3 = 0x00000207,
        IA32_MTRR_PHYSBASE4 = 0x00000208,
        IA32_MTRR_PHYSMASK4 = 0x00000209,
        IA32_MTRR_PHYSBASE5 = 0x0000020A,
        IA32_MTRR_PHYSMASK5 = 0x0000020B,
        IA32_MTRR_PHYSBASE6 = 0x0000020C,
        IA32_MTRR_PHYSMASK6 = 0x0000020D,
        IA32_MTRR_PHYSBASE7 = 0x0000020E,
        IA32_MTRR_PHYSMASK7 = 0x0000020F,
        // ... up to IA32_MTRRCAP::VCNT

        // Fixed Range MTRRs
        IA32_MTRR_FIX64K_00000 = 0x00000250,
        IA32_MTRR_FIX16K_80000 = 0x00000258,
        IA32_MTRR_FIX16K_A0000 = 0x00000259,
        IA32_MTRR_FIX4K_C0000 = 0x00000268,
        IA32_MTRR_FIX4K_C8000 = 0x00000269,
        IA32_MTRR_FIX4K_D0000 = 0x0000026A,
        IA32_MTRR_FIX4K_D8000 = 0x0000026B,
        IA32_MTRR_FIX4K_E0000 = 0x0000026C,
        IA32_MTRR_FIX4K_E8000 = 0x0000026D,
        IA32_MTRR_FIX4K_F0000 = 0x0000026E,
        IA32_MTRR_FIX4K_F8000 = 0x0000026F,

        IA32_PAT = 0x00000277,

        IA32_MTRR_DEF_TYPE = 0x000002FF,

        // x86-64 specific MSRs
        IA32_EFER = 0xc0000080,          // extended feature register
        IA32_STAR = 0xc0000081,          // Legacy mode SYSCALL target
        IA32_LSTAR = 0xc0000082,         // Long mode SYSCALL target
        IA32_CSTAR = 0xc0000083,         // Compat mode SYSCALL target
        IA32_FMASK = 0xc0000084,         // EFLAGS mask for SYSCALL
        IA32_FS_BASE = 0xc0000100,       // 64bit FS base
        IA32_GS_BASE = 0xc0000101,       // 64bit GS base
        IA32_KERNEL_GSBASE = 0xc0000102, // SwapGS GS shadow

        IA32_TSC_AUX = 0xc0000103, // Auxiliary TSC
    };

    // IA32_EFER bits
    constexpr uint64_t IA32_EFER_SCE = (1 << 0);    // SYSCALL / SYSRET
    constexpr uint64_t IA32_EFER_LME = (1 << 8);    // Long mode enable
    constexpr uint64_t IA32_EFER_LMA = (1 << 10);   // Long mode active (read-only)
    constexpr uint64_t IA32_EFER_NX = (1 << 11);    // No execute enable
    constexpr uint64_t IA32_EFER_SVME = (1 << 12);  // Enable virtualization
    constexpr uint64_t IA32_EFER_LMSLE = (1 << 13); // Long mode segment limit enable
    constexpr uint64_t IA32_EFER_FFXSR = (1 << 14); // Enable fast FXSAVE/FXRSTOR

    static inline uint64_t ReadMsr(Msr msr)
    {
        uint32_t low, high;
        asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
        return (uint64_t(high) << 32) | low;
    }

    static inline void WriteMsr(Msr msr, uint64_t value)
    {
        const uint32_t low = value & 0xFFFFFFFF;
        const uint32_t high = value >> 32;

        asm volatile("wrmsr" : : "c"(msr), "a"(low), "d"(high) : "memory");
    }

    // I/O ports
    static inline void x86_outb(uint16_t port, uint8_t value)
    {
        asm volatile("outb %1, %0" : : "dN"(port), "a"(value));
    }

    static inline void x86_outw(uint16_t port, uint16_t value)
    {
        asm volatile("outw %1, %0" : : "dN"(port), "a"(value));
    }

    static inline void x86_outl(uint16_t port, uint32_t value)
    {
        asm volatile("outl %1, %0" : : "dN"(port), "a"(value));
    }

    static inline uint8_t x86_inb(uint16_t port)
    {
        uint8_t ret;
        asm volatile("inb %1, %0" : "=a"(ret) : "dN"(port));
        return ret;
    }

    static inline uint16_t x86_inw(uint16_t port)
    {
        uint16_t ret;
        asm volatile("inw %1, %0" : "=a"(ret) : "dN"(port));
        return ret;
    }

    static inline uint32_t x86_inl(uint16_t port)
    {
        uint32_t ret;
        asm volatile("inl %1, %0" : "=a"(ret) : "dN"(port));
        return ret;
    }

    // Invalidate page tables for the specified address
    static inline void x86_invlpg(const void* virtualAddress)
    {
        asm volatile("invlpg (%0)" : : "r"(virtualAddress) : "memory");
    }

    // GDT / Segment Descriptor
    struct GdtDescriptor
    {
        uint16_t limit;
        uint16_t base;
        uint16_t flags1;
        uint16_t flags2;
    } __attribute__((aligned(8)));

    static_assert(sizeof(GdtDescriptor) == 8, "GdtDescriptor has unexpected size");

    struct GdtPtr
    {
        uint16_t size;
        void* address;
    } __attribute__((packed));

    static inline void x86_lgdt(const GdtPtr& gdt)
    {
        asm volatile("lgdt %0" : : "m"(gdt));
    }

    // IDT / Interrupt Descriptor
    struct IdtDescriptor
    {
        uint16_t offset_low;
        uint16_t selector;
        uint16_t flags;
        uint16_t offset_mid;
        uint32_t offset_high;
        uint32_t reserved;
    } __attribute__((aligned(16)));

    static_assert(sizeof(IdtDescriptor) == 16, "IdtDescriptor has unexpected size");

    struct IdtPtr
    {
        uint16_t size;
        void* address;
    } __attribute__((packed));

    static inline void x86_lidt(const IdtPtr& idt)
    {
        asm volatile("lidt %0" : : "m"(idt));
    }

    // Task State Segment
    //
    // There is a hardware constraint where we have to make sure that a TSS doesn't cross
    // page boundary. If that happen, invalid data might be loaded during a task switch.
    // Aligning the TSS to 128 bytes is enough to ensure that (128 > sizeof(Tss)).
    //
    // TSS is hard, see http://www.os2museum.com/wp/the-history-of-a-security-hole/
    struct Tss
    {
        uint32_t reserved0;
        uint64_t rsp0; // rsp when entering ring 0
        uint64_t rsp1; // rsp when entering ring 1
        uint64_t rsp2; // rsp when entering ring 2
        uint64_t reserved1;
        // The next 7 entries are the "Interrupt Stack Table"
        // Here we can define stack pointers to use when handling interrupts.
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

    } __attribute__((packed, aligned(128)));

    static_assert(sizeof(Tss) == 0x80, "Tss has unexpected size");

    static inline void x86_load_task_register(uint16_t selector)
    {
        asm volatile("ltr %0" : : "r"(selector));
    }

    ///////////////////////////////////////////////////////////////////////////
    // Interrupts
    ///////////////////////////////////////////////////////////////////////////

    // Enable interrupts for the current CPU
    static inline void EnableInterrupts()
    {
        asm volatile("sti" ::: "memory");
    }

    // Disable interrupts for the current CPU
    static inline void DisableInterrupts()
    {
        asm volatile("cli" ::: "memory");
    }

    // Are interrupts enabled for the current CPU?
    static inline bool InterruptsEnabled()
    {
        uint64_t flags;
        asm volatile("pushf; pop %0;" : "=r"(flags)::"memory");
        return flags & EFLAGS_IF ? true : false;
    }

    ///////////////////////////////////////////////////////////////////////////
    // CPU
    ///////////////////////////////////////////////////////////////////////////

    static inline void CpuPause()
    {
        __builtin_ia32_pause();
    }

} // namespace mtl
