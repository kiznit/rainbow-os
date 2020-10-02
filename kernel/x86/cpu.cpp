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
#include "apic.hpp"
#include "pit.hpp"
#include <metal/crt.hpp>
#include <metal/helpers.hpp>
#include <metal/log.hpp>
#include <metal/x86/interrupt.hpp>
#include <kernel/pmm.hpp>
#include <kernel/vmm.hpp>


int g_cpuCount;
Cpu g_cpus[MAX_CPU];


// TODO: disgusting use of the PIT, can we do better?
static PIT s_pit;


struct TrampolineContext
{
    volatile uint32_t flag;         // Track progress within the trampoline
    uint32_t          cr3;          // Page table for the processor. This must be in the first 4GB of memory.
    void*             stack;        // Kernel stack
    void*             entryPoint;   // Kernel entry point for the processor.
};


static void* smp_install_trampoline()
{
    extern const char SmpTrampolineStart[];
    extern const char SmpTrampolineEnd[];

    void* trampoline = (void*)(uintptr_t)pmm_allocate_frames_low(1);

    // TODO: we get away with not mapping the trampoline in virtual memory
    // because we have identity-mapped the first 4GB at boot time. We
    // might want to revisit how this is done. Simply mapping the page
    // doesn't work as the STARTUP IPI calls require the physical page and
    // I am too lazy to figure out how to return 2 values (physical and virtual
    // address) at once.

    const auto trampolineSize = SmpTrampolineEnd - SmpTrampolineStart;

    memcpy(trampoline, SmpTrampolineStart, trampolineSize);

    return trampoline;
}


static void smp_start(TrampolineContext* context)
{
    context->flag = 3;

    for (;;);
}



static bool cpu_start(void* trampoline, int cpuIndex)
{
    const Cpu& cpu = g_cpus[cpuIndex];
    Log("    Start CPU %d: id = %d, apic = %d, enabled = %d, bootstrap = %d\n", cpuIndex, cpu.id, cpu.apicId, cpu.enabled, cpu.bootstrap);
    if (cpu.bootstrap)
    {
        Log("        This is the current cpu, it is already running\n");
        return true;
    }

    // TODO: we have to make sure CR3 is in the lower 4GB for x86_64. For now we assert... :(
    assert(x86_get_cr3() < 0x100000000ull);

    // TODO: we should be creating a new idle task here, not just a stack
    // TODO: why doesn't it work with vmm_allocate_pages()? We get a PAGEFAULT 0x0B in the trampoline code... :(
    //void* stack = advance_pointer(vmm_allocate_pages(1), MEMORY_PAGE_SIZE);
    //void* stack = advance_pointer(trampoline, 0x0F00);
    void* stack = (void*)(pmm_allocate_frames_low(1) + MEMORY_PAGE_SIZE);
    Log("stack allocated at %p\n", stack);

    // Setup trampoline
    auto context = (TrampolineContext*)((uintptr_t)trampoline + 0x0F00);
    context->flag = 0;
    context->cr3 = x86_get_cr3();
    context->stack = stack;
    context->entryPoint = (void*)smp_start;

    *((uint32_t*)context->stack - 1) = 65;

    Log("stack: %p, entry %p\n", context->stack, context->entryPoint);

    // Send init IPI
    // TODO: we should do this in parallel for all APs so that the 10 ms wait is not serialized
    Log("        Sending INIT IPI\n");
    apic_write(APIC_ICR1, cpu.apicId << 24);    // IPI destination
    apic_write(APIC_ICR0, 0x4500);              // Send "init" command

    // Wait 10 ms
    s_pit.InitCountdown(10);
    while (!s_pit.IsCountdownExpired())
    {
        // Nothing
    }

    // Send startup IPI
    Log("        Sending 1st STARTUP IPI\n");
    const int vector = (uintptr_t)trampoline >> 12; // CPU will start execution at 000vv000h (vector = page number)
    assert(vector < 0x100);                         // Ensure trampoline is in low memory
    apic_write(APIC_ICR1, cpu.apicId << 24);        // IPI destination
    apic_write(APIC_ICR0, 0x4600 | vector);         // Send "startup" command

    // Poll flag for 1ms
    s_pit.InitCountdown(1);
    while (!context->flag && !s_pit.IsCountdownExpired())
    {
        // Nothing
    }

    if (!context->flag)
    {
        // Send 2nd startup IPI
        Log("        Sending 2nd STARTUP IPI\n");
        apic_write(APIC_ICR1, cpu.apicId << 24);    // IPI destination
        apic_write(APIC_ICR0, 0x4600 | vector);     // Send "startup" command

        // Poll for 1s
        for (int i = 0; i != 100; ++i)
        {
            s_pit.InitCountdown(10);

            while (!context->flag && !s_pit.IsCountdownExpired())
            {
                // Nothing
            }

            if (context->flag)
            {
                break;
            }
        }
    }

    // Wait until smp_start() runs
    while (context->flag != 3);

    Log("        FLAG: %x\n", context->flag);

    return true;
}


void cpu_smp_init()
{
    // NOTE: we can't have any interrupt enabled during SMP initialization!
    assert(!interrupt_enabled());

    void* trampoline = smp_install_trampoline();

    for (int i = 0; i != g_cpuCount; ++i)
    {
        cpu_start(trampoline, i);
    }

    pmm_free_frames((uintptr_t)trampoline, 1);
}
