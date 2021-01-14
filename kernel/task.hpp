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

#include <kernel/x86/cpu.hpp>

#if defined(__i386__)
#include <kernel/x86/ia32/interrupt.hpp>
#include <kernel/x86/ia32/task.hpp>
#elif defined(__x86_64__)
#include <kernel/x86/x86_64/interrupt.hpp>
#include <kernel/x86/x86_64/task.hpp>
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
        STATE_SLEEP,        // 3 - Task is sleeping until 'sleepUntilNs'
        STATE_ZOMBIE,       // 4 - Task died, but has not been destroyed / freed yet
        STATE_IPC_SEND,     // 5 - IPC send phase
        STATE_IPC_RECEIVE,  // 6 - IPC receive phase
        STATE_SEMAPHORE,    // 7 - Task is blocked on a semaphore
    };

    // Allocate / free a task
    void* operator new(size_t size);
    void* operator new(size_t size, void* task) { (void)size; return task; }
    void operator delete(void* p);

    // Get task by id, returns null if not found
    static Task* Get(Id id);

    Task();
    Task(EntryPoint entryPoint, int flags, const void* args, size_t sizeArgs);

    // TODO: can we unify/simplify/generalize the next two constructors? What about using variadic template parameters for args?
    template<typename T>
    Task(void (*entryPoint)(Task* task, const T* args), const T* args, int flags)
    : Task(reinterpret_cast<EntryPoint>(entryPoint), flags, args, 0)
    {
    }

    template<typename T>
    Task(void (*entryPoint)(Task* task, T* args), const T& args, int flags)
    : Task(reinterpret_cast<EntryPoint>(entryPoint), flags, &args, sizeof(args))
    {
    }

    ~Task();

    // Unlike std::thread, tasks are not owners of execution threads. They are instead
    // the thread themselves... So it doesn't make sense to allow copy / move semantics.
    // This is unless we want to implement fork() that way one day.
    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

    // Idle loop
    static void Idle();


//private: // TODO

    Id                  m_id;                   // Task ID
    State               m_state;                // Scheduling state
    WaitQueue*          m_queue;                // Where does this task live?

    TaskRegisters*      m_context;              // Saved context (on the task's stack)

    // TODO: use a shared pointer here to simplify code
    PageTable           m_pageTable;            // Page table
    uint64_t            sleepUntilNs;           // Sleep until this time (clock time in nanoseconds)

    void*               GetKernelStackTop() const   { return (void*)(this + 1); }
    void*               GetKernelStack() const      { return (char*)this + STACK_PAGE_COUNT * MEMORY_PAGE_SIZE; }

    void*               m_userStackTop;         // Top of user stack
    void*               m_userStackBottom;      // Bottom of user stack

    // TODO: move IPC WaitQueue outside the TCB?
    WaitQueue           m_ipcSenders;           // List of tasks blocked on ipc_call
    WaitQueue           m_ipcWaitReply;         // List of tasks waiting on a reply after ipc_call()
    // TODO: move IPC virtual registers out of TCB and map them in user space (UTCB, gs:0 in userspace)
    ipc_endpoint_t      m_ipcPartner;           // Who is our IPC partner?
    uintptr_t           m_ipcRegisters[64];     // Virtual registers for IPC
    FpuState            m_fpuState;             // FPU state

    // Return whether or not this task is blocked
    bool IsBlocked() const { return m_state >= STATE_SLEEP; }

    // Platform specific task-switching
    static void ArchSwitch(Task* currentTask, Task* newTask);


private:

    // Platform specific initialization
    void ArchInit(EntryPoint entryPoint, const void* args);

    // Entry point for new tasks
    static void Entry(Task* task, EntryPoint entryPoint, const void* args) noexcept __attribute__((noreturn));
};


#endif
