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

    typedef void (*EntryPoint)(Task* task, void* args);

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
        STATE_CALL,         // 3 - IPC: Client task is blocked on ipc_call
        STATE_WAIT,         // 4 - IPC: Service task is blocked on ipc_wait
        STATE_REPLY,        // 5 - IPC: Client task is blocked waiting for a reply
        STATE_SEMAPHORE,    // 6 - Task is blocked on a semaphore
    };

    // Get task by id, returns null if not found
    static Task* Get(Id id);
    // Initialize task 0

    static Task* InitTask0();       // TODO: Can we eliminate?

    // Spawn a new kernel task
    static Task* Create(EntryPoint entryPoint, const void* args, int flags);


    Id                  id;                 // Task ID
    State               state;              // Scheduling state
    Task*               next;               // Next task in list
    WaitQueue*          queue;              // Where does this task live?

    TaskRegisters*      context;            // Saved context (on the task's stack)
    PageTable           pageTable;          // Page table

    uintptr_t           kernelStackTop;     // Top of kernel stack
    uintptr_t           kernelStackBottom;  // Bottom of kernel stack

    uintptr_t           userStackTop;       // Top of user stack
    uintptr_t           userStackBottom;    // Bottom of user stack

    // TODO: move IPC WaitQueue outside the TCB?
    WaitQueue           ipcCallers;         // List of tasks blocked on ipc_call
    WaitQueue           ipcWaitReply;       // List of tasks waiting on a reply after ipc_call()
    // TODO: move IPC virtual registers out of TCB and map them in user space (UTCB, gs:0 in userspace)
    uintptr_t           ipcRegisters[64];   // Virtual registers for IPC

    // Return whether or not this task is blocked
    bool IsBlocked() const { return this->state >= STATE_CALL; }

    // Platform specific task-switching
    static void Switch(Task* currentTask, Task* newTask);


private:

    // Platform specific initialization
    static bool Initialize(Task* task, EntryPoint entryPoint, const void* args);

    // Entry point for new tasks.
    static void Entry();

    // Exit point for tasks that exit normally (returning from their task function).
    static void Exit();
};


#endif
