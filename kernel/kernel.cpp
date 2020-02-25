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
#include <rainbow/boot.hpp>
#include "elf.hpp"
#include "interrupt.hpp"
#include "thread.hpp"
#include "usermode.hpp"


// Machine abstraction
Scheduler*              g_scheduler;
ITimer*                 g_timer;
PhysicalMemoryManager*  g_pmm;
VirtualMemoryManager*   g_vmm;



// //TODO: temp
// #include "mutex.hpp"
// static Mutex g_mutex;

// static void ThreadFunction(void* args)
// {
//     const char* string = (char*)args;
//     for (;;)
//     {
//         g_mutex.Lock();
//         Log(string);
//         g_mutex.Unlock();
//     }
// }


// static void Test()
// {
//     Thread::Create(ThreadFunction, (void*)"1", Thread::CREATE_SHARE_USERSPACE);
//     Thread::Create(ThreadFunction, (void*)"2", Thread::CREATE_SHARE_USERSPACE);
//     ThreadFunction((void*)"0");
// }


static void LoadGo(void* args)
{
    const BootInfo* bootInfo = (BootInfo*)args;

    const physaddr_t goAddress = bootInfo->initrdAddress;
    const physaddr_t goSize = bootInfo->initrdSize;

    Log("Go at %X, size is %X\n", goAddress, goSize);

    const physaddr_t entryPoint = elf_map(goAddress, goSize);
    if (!entryPoint)
    {
        Fatal("Could not load / start GO process\n");
    }

    Log("Go entry point at %X\n", entryPoint);

// TODO: use constants for these, do not check for arch!
#if defined(__i386__)
    void* stack = (void*)0xE0000000; // TODO: should be 0xF0000000
#elif defined(__x86_64__)
    void* stack = (void*)0x0000800000000000;
#endif

    JumpToUserMode((UserSpaceEntryPoint)entryPoint, stack);
}


// Start the "Go" process.
static void StartGo(BootInfo* bootInfo)
{
    Thread::Create(LoadGo, bootInfo, 0);
}



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

    // TODO: free all MemoryType_Bootloader memory once we are done with BootInfo data

    g_scheduler->Init();

    // TODO: we want to free the current thread (#0) and its stack (_boot_stack - _boot_stack_top)

    StartGo(bootInfo);

    //Test();

    for(;;)
    {
        //Log("*");
    }

    return 0;
}
