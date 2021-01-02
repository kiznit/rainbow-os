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
#include <kernel/reent.hpp>
#include <kernel/kernel.hpp>
#include "console.hpp"
#include "usermode.hpp"


// Machine abstraction
ITimer* g_timer;

// Big Kernel Lock
// todo: do not use a big kernel lock
Spinlock g_bigKernelLock;


// TODO: haxxor until we have way to locate services
extern std::atomic_int s_nextTaskId;


// TODO: we might want to put this in some separate "discardable" segment
static BootInfo s_bootInfo;


extern "C" int kernel_main(BootInfo* bootInfo)
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
    bootInfo = &s_bootInfo;

    // Initialize kernel reentrancy logic
    reent_init();

    // The very first thing we want to do is make sure we are able to log information.
    // This is critical for debugging the kernel initialization code.
    console_early_init(bootInfo->framebuffers);
    Log("early console : check!\n");

    g_bigKernelLock.Lock();

    // Machine specific initialization
    machine_init(bootInfo);
    Log("machine       : check!\n");

    usermode_init();
    Log("usermode      : check!\n");

    // TODO: free all MemoryType_Bootloader memory once we are done with BootInfo data

    sched_initialize();
    Log("scheduler     : check!\n");

    // TODO: haxxor: we don't have a way to locate services yet, so we start them at a known id
    s_nextTaskId = 50;

    // TODO: can we make "go" launch the logger?
    usermode_spawn(&bootInfo->logger);

    usermode_spawn(&bootInfo->go);

    Task::Idle();

    return 0;
}
