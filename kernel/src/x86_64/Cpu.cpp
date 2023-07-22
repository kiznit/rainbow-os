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

#include "Cpu.hpp"
#include <cstring>

Cpu::Cpu()
{
    m_gsData.cpu = this;
}

void Cpu::Initialize()
{
    InitGdt();
    InitTss();

    LoadGdt();
    LoadTss();

    m_idt.Load();

    // Setup GS MSRs - make sure to do this *after* loading FS/GS. This is
    // because loading FS/GS on Intel will clear the FS/GS bases.
    mtl::WriteMsr(mtl::Msr::IA32_GS_BASE, (uintptr_t)&m_gsData); // Current active GS base
    mtl::WriteMsr(mtl::Msr::IA32_KERNEL_GSBASE, 0);              // The other GS base for swapgs
}

void Cpu::InitGdt()
{
    // 0x00 - Null Descriptor
    m_gdt[0].limit = 0x0000;  // Limit ignored in 64 bits mode
    m_gdt[0].base = 0x0000;   // Base ignored in 64 bits mode
    m_gdt[0].flags1 = 0x0000; // P + DPL 0 + S + Code + Read
    m_gdt[0].flags2 = 0x0000; // L (64 bits)

    // 0x08 - Kernel Code Segment Descriptor
    m_gdt[1].limit = 0x0000;  // Limit ignored in 64 bits mode
    m_gdt[1].base = 0x0000;   // Base ignored in 64 bits mode
    m_gdt[1].flags1 = 0x9A00; // P + DPL 0 + S + Code + Read
    m_gdt[1].flags2 = 0x0020; // L (64 bits)

    // 0x10 - Kernel Data Segment Descriptor
    m_gdt[2].limit = 0x0000;  // Limit ignored in 64 bits mode
    m_gdt[2].base = 0x0000;   // Base ignored in 64 bits mode
    m_gdt[2].flags1 = 0x9200; // P + DPL 0 + S + Data + Write
    m_gdt[2].flags2 = 0x0000; // Nothing

    // 0x18 - User Data Segment Descriptor
    m_gdt[3].limit = 0x0000;  // Limit ignored in 64 bits mode
    m_gdt[3].base = 0x0000;   // Base ignored in 64 bits mode
    m_gdt[3].flags1 = 0xF200; // P + DPL 3 + S + Data + Write
    m_gdt[3].flags2 = 0x0000; // Nothing

    // 0x20 - User Code Segment Descriptor
    m_gdt[4].limit = 0x0000;  // Limit ignored in 64 bits mode
    m_gdt[4].base = 0x0000;   // Base ignored in 64 bits mode
    m_gdt[4].flags1 = 0xFA00; // P + DPL 3 + S + Code + Read
    m_gdt[4].flags2 = 0x0020; // L (64 bits)

    // 0x28 - TSS - low
    const auto tss_base = (uintptr_t)&m_tss;
    const auto tss_limit = sizeof(m_tss) - 1;
    m_gdt[5].limit = tss_limit;                                       // Limit (15:0)
    m_gdt[5].base = (uint16_t)tss_base;                               // Base (15:0)
    m_gdt[5].flags1 = (uint16_t)(0xE900 + ((tss_base >> 16) & 0xFF)); // P + DPL 3 + TSS + base (23:16)
    m_gdt[5].flags2 = (uint16_t)((tss_base >> 16) & 0xFF00);          // Base (31:24)

    // 0x30 - TSS - high
    m_gdt[6].limit = (uint16_t)(tss_base >> 32); // Base (47:32)
    m_gdt[6].base = (uint16_t)(tss_base >> 48);  // Base (63:32)
    m_gdt[6].flags1 = 0x0000;
    m_gdt[6].flags2 = 0x0000;
}

void Cpu::InitTss()
{
    memset(&m_tss, 0, sizeof(m_tss));
    m_tss.iomap = 0xdfff; // For now, point beyond the TSS limit (no iomap)
}

void Cpu::LoadGdt()
{
    const mtl::GdtPtr gdtPtr{sizeof(m_gdt) - 1, m_gdt};
    mtl::x86_lgdt(gdtPtr);

    asm volatile("pushq %0\n"
                 "pushq $1f\n"
                 "lretq\n"
                 "1:\n"
                 :
                 : "i"(Selector::KernelCode)
                 : "memory");

    asm volatile("movl %0, %%ds\n"
                 "movl %0, %%es\n"
                 "movl %1, %%fs\n"
                 "movl %1, %%gs\n"
                 "movl %0, %%ss\n"
                 :
                 : "r"(Selector::KernelData), "r"(Selector::Null)
                 : "memory");
}

void Cpu::LoadTss()
{
    mtl::x86_load_task_register(static_cast<uint16_t>(Selector::Tss));
}
