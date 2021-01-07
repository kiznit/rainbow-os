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

#include "smp.hpp"
#include <cassert>
#include <cstring>
#include <metal/helpers.hpp>
#include <metal/log.hpp>
#include <metal/x86/interrupt.hpp>
#include <kernel/biglock.hpp>
#include <kernel/kernel.hpp>
#include <kernel/pmm.hpp>
#include <kernel/scheduler.hpp>
#include <kernel/task.hpp>
#include "apic.hpp"
#include "console.hpp"
#include "cpu.hpp"
#include "pit.hpp"

extern IdtPtr IdtPtr; // todo: ugly


// TODO: disgusting use of the PIT, can we do better?
static PIT s_pit;


struct TrampolineContext
{
    volatile uint32_t flag;         // Track progress within the trampoline
    uint32_t          cr3;          // Page table for the processor. This must be in the first 4GB of memory.
    void*             stack;        // Kernel stack
    void*             entryPoint;   // Kernel entry point for the processor.
    Cpu*              cpu;          // CPU information
    Task*             task;         // Initial task
    uint64_t          pat;          // value for MSR_PAT
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

    // We store TrampolineContext at 0x0F00, so make sure trampoline fits
    assert(trampolineSize < 0x0F00);

    memcpy(trampoline, SmpTrampolineStart, trampolineSize);

    return trampoline;
}


// Newly started processors jump here from the real mode trampoline
static void smp_entry(TrampolineContext* context)
{
    assert(!interrupt_enabled());

    // Make sure to init MSR_PAT before writing anything to the screen!
    x86_write_msr(MSR_PAT, context->pat);

    auto cpu = context->cpu;
    cpu->Initialize();

    // We can only get the lock once per-CPU data is accessible through cpu_get_data().
    // This is currently done in apic_init().
    g_bigKernelLock.lock();

    auto task = context->task;
    cpu->task = task;
    task->state = Task::STATE_RUNNING;
    task->pageTable.cr3 = context->cr3;      // TODO: platform specific code does not belong here

    x86_lidt(IdtPtr);

    Log("CPU %d started, task %d\n", cpu->id, task->id);

    context->flag = 3;

    Task::Idle();
}


static bool smp_start_cpu(void* trampoline, const Cpu& cpu)
{
    Log("    Start CPU: id = %d, apic = %d, enabled = %d, bootstrap = %d\n", cpu.id, cpu.apicId, cpu.enabled, cpu.bootstrap);

    if (cpu.bootstrap)
    {
        Log("        This is the current cpu, it is already running\n");
        return true;
    }

    // TODO: we have to make sure CR3 is in the lower 4GB for x86_64. For now we assert... :(
    assert(x86_get_cr3() < 0x100000000ull);

    // Create a new task for the CPU
    const auto task = Task::Allocate();

    // Setup trampoline
    auto context = (TrampolineContext*)((uintptr_t)trampoline + 0x0F00);
    context->flag = 0;
    context->cr3 = x86_get_cr3();
    context->stack = task->GetKernelStack();
    context->entryPoint = (void*)smp_entry;
    context->cpu = const_cast<Cpu*>(&cpu);
    context->task = task;
    context->pat = x86_read_msr(MSR_PAT);

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

    // TODO: can we harden this and make sure we don't start the same processor twice (or that if we do, it's not problem)?
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

    // We need to release the lock so that Task::Entry() can proceed
    g_bigKernelLock.unlock();

    // Wait until smp_start() runs
    while (context->flag != 3);

    // Take the lock back
    g_bigKernelLock.lock();

    return true;
}


void smp_init()
{
    // NOTE: we can't have any interrupt enabled during SMP initialization!
    assert(!interrupt_enabled());

    void* trampoline = smp_install_trampoline();

    for (const auto& cpu: g_cpus)
    {
        if (cpu->enabled)
        {
            smp_start_cpu(trampoline, *cpu);
        }
    }

    pmm_free_frames((uintptr_t)trampoline, 1);
}
