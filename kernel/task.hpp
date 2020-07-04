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

#ifndef _RAINBOW_KERNEL_TASK_HPP
#define _RAINBOW_KERNEL_TASK_HPP

#include <kernel/pagetable.hpp>
#include <kernel/config.hpp>
#include <rainbow/ipc.h>
#include "waitqueue.hpp"

#if defined(__i386__)
#include "x86/ia32/cpu.hpp"
#include "x86/ia32/task.hpp"
#elif defined(__x86_64__)
#include "x86/x86_64/cpu.hpp"
#include "x86/x86_64/task.hpp"
#endif


class Task
{
public:
    typedef int Id;

    typedef void (*EntryPoint)(Task* task, const void* args);

    enum Create
    {
        CREATE_SHARE_PAGE_TABLE = 1,    // The new task shares the page tables with the current one
    };

    enum State
    {
        STATE_INIT,         // 0 - Task is initializing
        STATE_RUNNING,      // 1 - Task is running
        STATE_READY,        // 2 - Task is ready to run

        // Blocked states
        STATE_IPC_SEND,     // 3 - IPC send phase
        STATE_IPC_RECEIVE,  // 4 - IPC receive phase
        STATE_SEMAPHORE,    // 5 - Task is blocked on a semaphore
    };

    // Get task by id, returns null if not found
    static Task* Get(Id id);
    // Initialize task 0

    static Task* InitTask0();       // TODO: Can we eliminate?

    // Spawn a new kernel task
    template<typename T, typename F>
    static Task* Create(F entryPoint, const T* args, int flags)
    {
        return CreateImpl(reinterpret_cast<EntryPoint>(entryPoint), flags, args, 0);
    }

    template<typename T, typename F>
    static Task* Create(F entryPoint, const T& args, int flags)
    {
        return CreateImpl(reinterpret_cast<EntryPoint>(entryPoint), flags, &args, sizeof(args));
    }


    Id                  id;                 // Task ID
    State               state;              // Scheduling state
    Task*               next;               // Next task in list
    WaitQueue*          queue;              // Where does this task live?

    TaskRegisters*      context;            // Saved context (on the task's stack)
    PageTable           pageTable;          // Page table

    void*               GetKernelStackTop() const   { return (void*)(this + 1); }
    void*               GetKernelStack() const      { return (char*)this + STACK_PAGE_COUNT * MEMORY_PAGE_SIZE; }

    void*               userStackTop;       // Top of user stack
    void*               userStackBottom;    // Bottom of user stack

    // TODO: move IPC WaitQueue outside the TCB?
    WaitQueue           ipcSenders;         // List of tasks blocked on ipc_call
    WaitQueue           ipcWaitReply;       // List of tasks waiting on a reply after ipc_call()
    // TODO: move IPC virtual registers out of TCB and map them in user space (UTCB, gs:0 in userspace)
    ipc_endpoint_t      ipcPartner;         // Who is our IPC partner?
    uintptr_t           ipcRegisters[64];   // Virtual registers for IPC
    FpuState            fpuState;           // FPU state

    // Return whether or not this task is blocked
    bool IsBlocked() const { return this->state >= STATE_IPC_SEND; }

    // Platform specific task-switching
    static void Switch(Task* currentTask, Task* newTask);


private:

    // Platform specific initialization
    static bool Initialize(Task* task, EntryPoint entryPoint, const void* args);

    // Create implementation
    static Task* CreateImpl(EntryPoint entryPoint, int flags, const void* args, size_t sizeArgs);

    // Entry point for new tasks.
    static void Entry(Task* task, EntryPoint entryPoint, const void* args) __attribute__((noreturn));
};


#endif
