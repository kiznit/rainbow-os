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

#include <cassert>
#include <kernel/biglock.hpp>
#include <kernel/kernel.hpp>
#include <rainbow/boot.hpp>
#include "acpi.hpp"
#include "apic.hpp"
#include "console.hpp"
#include "cpu.hpp"
#include "pit.hpp"
#include "smp.hpp"


static PIT s_pit;

void machine_init(BootInfo* bootInfo)
{
    // Initialize memory systems
    pmm_initialize((MemoryDescriptor*)bootInfo->descriptors, bootInfo->descriptorCount);
    vmm_initialize();
    Log("Memory        : check!\n");

    // We "initialize" ACPI because we need to access the APIC below
    acpi_init(bootInfo->acpiRsdp);
    Log("ACPI          : check!\n");

    // We "initialize" APIC because we want to properly initialize the
    // current CPU and we need the processor id to do so (that id is used
    // for the big kernel lock amongst other things).
    apic_init();
    Log("APIC          : check!\n");

    // We can only get the lock once per-CPU data is accessible through cpu_get_data().
    // This is currently done in apic_init().
    g_bigKernelLock.lock();

    console_init();
    Log("Console       : check!\n");

    smp_init();
    Log("SMP           : check!\n");

    interrupt_init();
    Log("Interrupt     : check!\n");
    assert(!interrupt_enabled());

    g_timer = &s_pit;
    Log("Timer         : check!\n");
}
