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

#ifndef _RAINBOW_KERNEL_THREAD_HPP
#define _RAINBOW_KERNEL_THREAD_HPP

#include <kernel/pagetable.hpp>

#if defined(__i386__)
#include "x86/ia32/thread.hpp"
#elif defined(__x86_64__)
#include "x86/x86_64/thread.hpp"
#endif


class Thread
{
public:
    typedef unsigned int Id;

    typedef void (*EntryPoint)(void* args);

    enum Create
    {
        CREATE_SHARE_VM,        // The new thread shares virtual memory (page tables) with the current one
        CREATE_SHARE_USERSPACE  // Unless specified, user space page tables will not be shared - TODO: this is messy
    };

    enum State
    {
        STATE_INIT,         // Thread is initializing
        STATE_RUNNING,      // Thread is running
        STATE_READY,        // Thread is ready to run
        STATE_SUSPENDED,    // Thread is blocked on a semaphore
    };

    // Get thread by id, returns null if not found
    static Thread* Get(Id id);
    // Initialize thread 0

    static Thread* InitThread0();       // Can we eliminate?

    // Spawn a new kernel thread
    static Thread* Create(EntryPoint entryPoint, const void* args, int flags);


    Id                  id;                 // Thread ID
    State               state;              // Scheduling state
    ThreadRegisters*    context;            // Saved context (on the thread's stack)
    PageTable           pageTable;          // Page table

    const void*         kernelStackTop;     // Top of kernel stack
    const void*         kernelStackBottom;  // Bottom of kernel stack

    Thread*             next;               // Next thread in list


private:

    // Platform specific initialization
    static bool Initialize(Thread* thread, EntryPoint entryPoint, const void* args);

    // Entry point for new threads.
    static void Entry();

    // Exit point for threads that exit normally (returning from their thread function).
    static void Exit();
};


#endif
