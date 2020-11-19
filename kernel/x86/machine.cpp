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

#include <kernel/kernel.hpp>
#include <rainbow/boot.hpp>
#include "acpi.hpp"
#include "apic.hpp"
#include "console.hpp"
#include "pit.hpp"
#include "smp.hpp"


static PIT s_timer;

void machine_init(BootInfo* bootInfo)
{
    // Initialize memory systems
    pmm_initialize((MemoryDescriptor*)bootInfo->descriptors, bootInfo->descriptorCount);
    vmm_initialize();
    Log("Memory        : check!\n");

    cpu_init();
    Log("CPU           : check!\n");

    acpi_init(bootInfo->acpiRsdp);
    Log("ACPI          : check!\n");

    apic_init();
    Log("APIC          : check!\n");

    console_init();
    Log("Console       : check!\n");

    // NOTE: we can't have any interrupt enabled during SMP initialization!
    if (g_cpuCount > 1)
    {
        smp_init();
        Log("SMP           : check!\n");
    }

    interrupt_init();
    Log("Interrupt     : check!\n");
    assert(!interrupt_enabled());

    g_timer = &s_timer;
    Log("Timer         : check!\n");
}
