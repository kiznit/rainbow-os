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

#include <kernel/kernel.hpp>
#include "usermode.hpp"


// Machine abstraction
Scheduler*              g_scheduler;
ITimer*                 g_timer;
PhysicalMemoryManager*  g_pmm;
VirtualMemoryManager*   g_vmm;


// TODO: we might want to put this in some separate "discartable" segment
static BootInfo s_bootInfo;


extern "C" int kernel_main(BootInfo* bootInfo)
{
    if (!bootInfo || bootInfo->version != RAINBOW_BOOT_VERSION)
    {
        return -1;
    }

    // Copy BootInfo into kernel space so that it can easily be used to spawn initial user processes
    s_bootInfo = *bootInfo;
    bootInfo = &s_bootInfo;

    // Start initialization sequence
    console_init(bootInfo->framebuffers);
    Log("Console   : check!\n");

    cpu_init();
    Log("CPU       : check!\n");

    machine_init(bootInfo);
    Log("machine   : check!\n");

    interrupt_init();
    Log("interrupt : check!\n");

    usermode_init();
    Log("usermode  : check!\n");

    // TODO: free all MemoryType_Bootloader memory once we are done with BootInfo data

    g_scheduler->Init();

    usermode_spawn(&bootInfo->initrd);

    // TODO: we want to free the current thread (#0) and its stack (_boot_stack - _boot_stack_top)

    //Test();

    for(;;)
    {
        Log("K");
    }

    return 0;
}
