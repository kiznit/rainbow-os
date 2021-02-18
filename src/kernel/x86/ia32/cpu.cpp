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

#include <kernel/x86/cpu.hpp>
#include <kernel/x86/selectors.hpp>
#include <metal/helpers.hpp>


extern "C" void sysenter_entry();


void Cpu::InitGdt()
{
    // Entry 0x00 is the null descriptor

    // 0x08 - Kernel Code Segment Descriptor
    gdt[1].limit    = 0x0000;   // Limit (4 KB granularity, will be set in cpu_init() below)
    gdt[1].base     = 0x0000;   // Base = 0
    gdt[1].flags1   = 0x9A00;   // P + DPL 0 + S + Code + Read
    gdt[1].flags2   = 0x00C0;   // G + D (32 bits)

    // 0x10 - Kernel Data Segment Descriptor
    gdt[2].limit    = 0xFFFF;   // Limit = 0x100000 * 4 KB = 4 GB
    gdt[2].base     = 0x0000;   // Base = 0
    gdt[2].flags1   = 0x9200;   // P + DPL 0 + S + Data + Write
    gdt[2].flags2   = 0x00CF;   // G + B (32 bits) + limit 19:16

// TODO: how can we set lower limits for the user space segment descriptors?

    // 0x18 - User Code Segment Descriptor
    gdt[3].limit    = 0xFFFF;   // Limit = 0x100000 * 4 KB = 4 GB
    gdt[3].base     = 0x0000;   // Base = 0
    gdt[3].flags1   = 0xFA00;   // P + DPL 3 + S + Code + Read
    gdt[3].flags2   = 0x00CF;   // G + B (32 bits) + limit 19:16

    // 0x20 - User Data Segment Descriptor
    gdt[4].limit    = 0xFFFF;   // Limit = 0x100000 * 4 KB = 4 GB
    gdt[4].base     = 0x0000;   // Base = 0
    gdt[4].flags1   = 0xF200;   // P + DPL 3 + S + Data + Write
    gdt[4].flags2   = 0x00CF;   // G + B (32 bits) + limit 19:16

    // 0x28 - TSS
    const uintptr_t tss_base = (uintptr_t)tss;
    const uintptr_t tss_limit = sizeof(*tss) - 1;
    gdt[5].limit    = tss_limit;                                      // Limit (15:0)
    gdt[5].base     = (uint16_t)tss_base;                             // Base (15:0)
    gdt[5].flags1   = (uint16_t)(0xE900 + ((tss_base >> 16) & 0xFF)); // P + DPL 3 + TSS + base (23:16)
    gdt[5].flags2   = (uint16_t)((tss_base >> 16) & 0xFF00);          // Base (31:24)

    // 0x30 - Cpu data
    gdt[6].SetKernelData32((uintptr_t)this, sizeof(*this));

    // 0x38 - TLS

    // Set the CS limit of kernel code segment to what we need (and not higher)
    extern void* __etext[];
    const uint32_t limit = align_down((uintptr_t)__etext, MEMORY_PAGE_SIZE) >> MEMORY_PAGE_SHIFT;
    const int gdtIndex = GDT_KERNEL_CODE / sizeof(GdtDescriptor);
    gdt[gdtIndex].limit = limit & 0xFFFF;
    gdt[gdtIndex].flags2 |= (limit >> 16) & 0xF;
}


void Cpu::InitTss()
{
    // TSS
    tss->ss0 = GDT_KERNEL_DATA;
    tss->iomap = 0xdfff; // For now, point beyond the TSS limit (no iomap)
}


void Cpu::Initialize()
{
    // Load GDT
    const GdtPtr gdtptr = { 8 * sizeof(GdtDescriptor)-1, gdt };
    x86_lgdt(gdtptr);

    // Load code segment
    asm volatile (
        "pushl %0\n"
        "pushl $1f\n"
        "retf\n"
        "1:\n"
        : : "i"(GDT_KERNEL_CODE) : "memory"
    );

    // Load data segments
    asm volatile (
        "movl %0, %%ds\n"
        "movl %0, %%es\n"
        "movl %1, %%fs\n"
        "movl %0, %%ss\n"
        : : "r" (GDT_KERNEL_DATA), "r"(GDT_CPU_DATA) : "memory"
    );

    // Load TSS
    x86_load_task_register(GDT_TSS);

    // Enable SSE
    auto cr4 = x86_get_cr4();
    cr4 |= X86_CR4_OSFXSR | X86_CR4_OSXMMEXCPT;
    x86_set_cr4(cr4);

    // Configure sysenter
    x86_write_msr(MSR_SYSENTER_CS, GDT_KERNEL_CODE);
    x86_write_msr(MSR_SYSENTER_EIP, (uintptr_t)sysenter_entry);
}
