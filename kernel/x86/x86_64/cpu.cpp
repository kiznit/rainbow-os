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

#include <metal/arch.hpp>
#include <metal/crt.hpp>
#include <metal/x86/cpu.hpp>


// TODO: we will need one TSS per CPU

// There is a hardware constraint where we have to make sure that a TSS doesn't cross
// page boundary. If that happen, invalid data might be loaded during a task switch.
// Aligning the TSS to 128 bytes is enough to ensure that (128 > sizeof(Tss)).
Tss64 g_tss __attribute__((aligned(128)));;

static const uintptr_t tss_base = (uintptr_t)&g_tss;
static const uintptr_t tss_limit = sizeof(Tss) - 1;


struct GdtDescriptor
{
    uint16_t limit;
    uint16_t base;
    uint16_t flags1;
    uint16_t flags2;
};


struct GdtPtr
{
    uint16_t size;
    void* address;
} __attribute__((packed));


static GdtDescriptor GDT[] __attribute__((aligned(16))) =
{
    // 0x00 - Null Descriptor
    { 0, 0, 0, 0 },

    // 0x08 - Kernel Code Segment Descriptor
    {
        0x0000,     // Limit ignored in 64 bits mode
        0x0000,     // Base ignored in 64 bits mode
        0x9A00,     // P + DPL 0 + S + Code + Read
        0x0020,     // L (64 bits)
    },

    // 0x10 - Kernel Data Segment Descriptor
    {
        0x0000,     // Limit ignored in 64 bits mode
        0x0000,     // Base ignored in 64 bits mode
        0x9200,     // P + DPL 0 + S + Data + Write
        0x0000,     // Nothing
    },

    // 0x18 - User Code Segment Descriptor
    {
        0x0000,     // Limit ignored in 64 bits mode
        0x0000,     // Base ignored in 64 bits mode
        0xFA00,     // P + DPL 3 + S + Code + Read
        0x0020,     // L (64 bits)
    },

    // 0x20 - User Data Segment Descriptor
    {
        0x0000,     // Limit ignored in 64 bits mode
        0x0000,     // Base ignored in 64 bits mode
        0xF200,     // P + DPL 3 + S + Data + Write
        0x0000,     // Nothing
    },

    // 0x28 - TSS - low
    {
        tss_limit,                                      // Limit (15:0)
        (uint16_t)tss_base,                             // Base (15:0)
        (uint16_t)(0xE900 + ((tss_base >> 16) & 0xFF)), // P + DPL 3 + TSS + base (23:16)
        (uint16_t)((tss_base >> 16) & 0xFF00)           // Base (31:24)
    },
    // 0x28 - TSS - high
    {
        (uint16_t)(tss_base >> 32),                     // Base (47:32)
        (uint16_t)(tss_base >> 48),                     // Base (63:32)
        0x0000,
        0x0000
    }
};


static GdtPtr GdtPtr =
{
    sizeof(GDT)-1,
    GDT
};


void cpu_init()
{
    // Load GDT
    asm volatile ("lgdtq %0" : : "m" (GdtPtr) );

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
        "movl %0, %%fs\n"
        "movl %0, %%gs\n"
        "movl %0, %%ss\n"
        : : "r" (GDT_KERNEL_DATA) : "memory"
    );

    // TSS
    memset(&g_tss, 0, sizeof(g_tss));
    g_tss.iomap = 0xdfff; // For now, point beyond the TSS limit (no iomap)

    x86_load_task_register(GDT_TSS); // TSS descriptor
}
