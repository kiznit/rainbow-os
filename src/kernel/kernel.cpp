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

#include <kernel/biglock.hpp>
#include <kernel/console.hpp>
#include <kernel/kernel.hpp>
#include <kernel/scheduler.hpp>
#include <kernel/usermode.hpp>
#include <kernel/x86/smp.hpp>   // TODO: arch specific...


extern "C" void _fini();
extern "C" void _init();
static int kernel_main();

// Machine abstraction
IClock* g_clock;
ITimer* g_timer;

// We need a way to let the runtime know whether or not we are "early" in the startup
// sequence. For example, the global constructors might try to lock spinlocks, allocate
// memory, and so on... But at that point the runtime cannot ask for per-cpu data or
// for the current task since these don't exist yet. This flag is the current workaround.
bool g_isEarly = true;


// Big Kernel Lock
// TODO: do not use a big kernel lock
RecursiveSpinlock g_bigKernelLock;

// The scheduler
Scheduler g_scheduler;

// TODO: haxxor until we have a way to locate services
extern std::atomic_int s_nextTaskId;


// TODO: we might want to put this in some separate "discardable" segment
static BootInfo s_bootInfo;


extern "C" int _start_kernel(BootInfo* bootInfo)
{
    // Validate that the boot information is valid and as expected.
    if (!bootInfo || bootInfo->version != RAINBOW_BOOT_VERSION)
    {
        return -1;
    }

    // Copy boot parameters into kernel space so that we don't have to keep the
    // bootloader's memory around. Also new spawned tasks won't necessarily have
    // access to memory outside kernel space and they might be interested in the
    // boot parameters.
    s_bootInfo = *bootInfo;

    // Initialize early logging. This cannot allocate memory as we haven't
    // initialized the memory systems yet (pmm/vmm). Also note that global
    // constructors have not been called yet!
    console_early_init(s_bootInfo.framebuffers);
    Log("early console : check!\n");

    // Initialize memory systems
    pmm_initialize((MemoryDescriptor*)bootInfo->descriptors, bootInfo->descriptorCount);
    vmm_initialize();
    Log("Memory        : check!\n");

    // Call global constructors
    _init();

    // Machine specific initialization
    machine_init(&s_bootInfo);
    Log("machine       : check!\n");

    usermode_init();
    Log("usermode      : check!\n");

    g_scheduler.Initialize();
    Log("scheduler     : check!\n");

    // TODO: free all MemoryType_Bootloader memory once we are done with BootInfo data

    // Basic components are up and running.
    g_isEarly = false;

    smp_init();
    Log("SMP           : check!\n");

    // Execute kernel
    const int status = kernel_main();

    // Call global destructors
    _fini();

    return status;
}


static int kernel_main()
{
    // TODO: haxxor: we don't have a way to locate services yet, so we start them at a known id
    s_nextTaskId = 51;

    // TODO: can we make "go" launch the logger?
    usermode_spawn(&s_bootInfo.logger);

    usermode_spawn(&s_bootInfo.go);

    Task::Idle();

    return 0;
}
