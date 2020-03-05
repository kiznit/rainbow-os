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

#include <metal/arch.hpp>
#include <metal/crt.hpp>
#include <metal/helpers.hpp>
#include <metal/x86/cpu.hpp>

// TODO: we will need one TSS per CPU
PageAlignedTss g_tss;

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
        0x0000,     // Limit (4 KB granularity, will be set in cpu_init() below)
        0x0000,     // Base = 0
        0x9A00,     // P + DPL 0 + S + Code + Read
        0x00C0,     // G + D (32 bits)
    },

    // 0x10 - Kernel Data Segment Descriptor
    {
        0xFFFF,     // Limit = 0x100000 * 4 KB = 4 GB
        0x0000,     // Base = 0
        0x9200,     // P + DPL 0 + S + Data + Write
        0x00CF,     // G + B (32 bits) + limit 19:16
    },

// TODO: how can we set lower limits for the user space segment descriptors?

    // 0x18 - User Code Segment Descriptor
    {
        0xFFFF,     // Limit = 0x100000 * 4 KB = 4 GB
        0x0000,     // Base = 0
        0xFA00,     // P + DPL 3 + S + Code + Read
        0x00CF,     // G + D (32 bits) + limit 19:16
    },

    // 0x20 - User Data Segment Descriptor
    {
        0xFFFF,     // Limit = 0x100000 * 4 KB = 4 GB
        0x0000,     // Base = 0
        0xF200,     // P + DPL 3 + S + Data + Write
        0x00CF,     // G + B (32 bits) + limit 19:16
    },

    // 0x28 - TSS
    {
        tss_limit,                                      // Limit (15:0)
        (uint16_t)tss_base,                             // Base (15:0)
        (uint16_t)(0xE900 + ((tss_base >> 16) & 0xFF)), // P + DPL 3 + TSS + base (23:16)
        (uint16_t)((tss_base >> 16) & 0xFF00)           // Base (31:24)
    }
};


static GdtPtr GdtPtr =
{
    sizeof(GDT)-1,
    GDT
};


void cpu_init()
{
    // Set the CS limit to what we need (and not higher)
    extern void* _etext[];
    const uint32_t limit = align_down((uintptr_t)_etext, MEMORY_PAGE_SIZE) >> MEMORY_PAGE_SHIFT;
    const int gdtIndex = GDT_KERNEL_CODE / sizeof(GdtDescriptor);
    GDT[gdtIndex].limit = limit & 0xFFFF;
    GDT[gdtIndex].flags2 |= (limit >> 16) & 0xF;

    // Load GDT
    asm volatile ("lgdt %0" : : "m" (GdtPtr) );

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
        "movl %0, %%fs\n"
        "movl %0, %%gs\n"
        "movl %0, %%ss\n"
        : : "r" (GDT_KERNEL_DATA) : "memory"
    );

    // TSS
    memset(&g_tss, 0, sizeof(g_tss));
    g_tss.ss0 = GDT_KERNEL_DATA;
    g_tss.iomap = 0xdfff; // For now, point beyond the TSS limit (no iomap)

    x86_load_task_register(GDT_TSS); // TSS descriptor
}
