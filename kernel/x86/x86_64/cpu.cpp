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

#include "cpu.hpp"
#include <cstring>
#include <metal/arch.hpp>
#include <metal/x86/cpu.hpp>
#include <kernel/vmm.hpp>
#include <kernel/x86/selectors.hpp>
#include <kernel/x86/smp.hpp>

extern "C" void syscall_entry();


void cpu_init()
{
    // Keep the GDT in its own page to prevent information leak (spectre/meltdown)
    auto gdt = (GdtDescriptor*)vmm_allocate_pages(1);   // TODO: error handling

    static_assert(sizeof(PerCpu) <= MEMORY_PAGE_SIZE);
    auto percpu = (PerCpu*)vmm_allocate_pages(1);       // TODO: error handling

    auto tss = &percpu->tss64;

    // Entry 0x00 is the null descriptor

    // 0x08 - Kernel Code Segment Descriptor
    gdt[1].limit    = 0x0000;   // Limit ignored in 64 bits mode
    gdt[1].base     = 0x0000;   // Base ignored in 64 bits mode
    gdt[1].flags1   = 0x9A00;   // P + DPL 0 + S + Code + Read
    gdt[1].flags2   = 0x0020;   // L (64 bits)

    // 0x10 - Kernel Data Segment Descriptor
    gdt[2].limit    = 0x0000;   // Limit ignored in 64 bits mode
    gdt[2].base     = 0x0000;   // Base ignored in 64 bits mode
    gdt[2].flags1   = 0x9200;   // P + DPL 0 + S + Data + Write
    gdt[2].flags2   = 0x0000;   // Nothing

    // 0x18 - User Data Segment Descriptor
    gdt[3].limit    = 0x0000;   // Limit ignored in 64 bits mode
    gdt[3].base     = 0x0000;   // Base ignored in 64 bits mode
    gdt[3].flags1   = 0xF200;   // P + DPL 3 + S + Data + Write
    gdt[3].flags2   = 0x0000;   // Nothing

    // 0x20 - User Code Segment Descriptor
    gdt[4].limit    = 0x0000;   // Limit ignored in 64 bits mode
    gdt[4].base     = 0x0000;   // Base ignored in 64 bits mode
    gdt[4].flags1   = 0xFA00;   // P + DPL 3 + S + Code + Read
    gdt[4].flags2   = 0x0020;   // L (64 bits)

    // 0x28 - TSS - low
    const uintptr_t tss_base = (uintptr_t)tss;
    const uintptr_t tss_limit = sizeof(*tss) - 1;
    gdt[5].limit    = tss_limit;                                      // Limit (15:0)
    gdt[5].base     = (uint16_t)tss_base;                             // Base (15:0)
    gdt[5].flags1   = (uint16_t)(0xE900 + ((tss_base >> 16) & 0xFF)); // P + DPL 3 + TSS + base (23:16)
    gdt[5].flags2   = (uint16_t)((tss_base >> 16) & 0xFF00);          // Base (31:24)

    // 0x30 - TSS - high
    gdt[6].limit    = (uint16_t)(tss_base >> 32);                     // Base (47:32)
    gdt[6].base     = (uint16_t)(tss_base >> 48);                     // Base (63:32)
    gdt[6].flags1   = 0x0000;
    gdt[6].flags2   = 0x0000;

    // Load GDT
    const GdtPtr gdtptr =
    {
        7 * sizeof(GdtDescriptor)-1,
        gdt
    };

    x86_lgdt(gdtptr);

    // Load code segment
    asm volatile (
        "pushq %0\n"
        "pushq $1f\n"
        "retfq\n"
        "1:\n"
        : : "i"(GDT_KERNEL_CODE) : "memory"
    );

    // Load data segments
    asm volatile (
        "movl %0, %%ds\n"
        "movl %0, %%es\n"
        "movl %1, %%fs\n"
        "movl %1, %%gs\n"
        "movl %0, %%ss\n"
        : : "r" (GDT_KERNEL_DATA), "r" (GDT_NULL) : "memory"
    );

    // TSS
    tss->iomap = 0xdfff; // For now, point beyond the TSS limit (no iomap)
    x86_load_task_register(GDT_TSS);

    // Setup GS MSRs - make sure to do this *after* loading fs/gs. This is
    // because loading fs/gs on Intel will clear the GS bases.
    x86_write_msr(MSR_GS_BASE, (uintptr_t)percpu);  // Current active GS base
    x86_write_msr(MSR_KERNEL_GS_BASE, 0);           // The other GS base for swapgs

    // Initialize per-cpu data
    percpu->gdt = gdt;
    percpu->task = nullptr;
    percpu->tss = tss;
    percpu->cpu = &g_cpus[0];

    // Enable SSE
    auto cr4 = x86_get_cr4();
    cr4 |= X86_CR4_OSFXSR | X86_CR4_OSXMMEXCPT;
    x86_set_cr4(cr4);

    // Configure syscall / sysret
    x86_write_msr(MSR_STAR, (((uint64_t)GDT_USER_CODE - 16) << 48) | ((uint64_t)GDT_KERNEL_CODE << 32));
    x86_write_msr(MSR_LSTAR, (uintptr_t)syscall_entry);
    x86_write_msr(MSR_FMASK, X86_EFLAGS_IF | X86_EFLAGS_DF | X86_EFLAGS_RF | X86_EFLAGS_VM); // Same flags as sysenter + DF for convenience

    // Enable syscall
    uint64_t efer = x86_read_msr(MSR_EFER);
    efer |= EFER_SCE;
    x86_write_msr(MSR_EFER, efer);
}
