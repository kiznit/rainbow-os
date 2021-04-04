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

#include <memory>
#include <kernel/pagetable.hpp>
#include <kernel/config.hpp>
#include <rainbow/ipc.h>
#include "taskdefs.hpp"
#include "waitqueue.hpp"

#include <kernel/x86/cpu.hpp>

#if defined(__i386__)
#include <kernel/x86/ia32/interrupt.hpp>
#include <kernel/x86/ia32/task.hpp>
#elif defined(__x86_64__)
#include <kernel/x86/x86_64/interrupt.hpp>
#include <kernel/x86/x86_64/task.hpp>
#endif

#include <metal/memory.hpp>


class Task
{
public:
    typedef int Id;

    typedef void (*EntryPoint)(Task* task, const void* args);

    // Allocate / free a task
    void* operator new(size_t size);
    void* operator new(size_t size, void* task) { (void)size; return task; }
    void operator delete(void* p);

    // Get task by id, returns null if not found
    static Task* Get(Id id);

    Task(const std::shared_ptr<PageTable>& pageTable);
    Task(EntryPoint entryPoint, const void* args, size_t sizeArgs, const std::shared_ptr<PageTable>& pageTable);

    // TODO: can we unify/simplify/generalize the next two constructors? What about using variadic template parameters for args?
    template<typename T>
    Task(void (*entryPoint)(Task* task, const T* args), const T* args, const std::shared_ptr<PageTable>& pageTable)
    : Task(reinterpret_cast<EntryPoint>(entryPoint), args, 0, pageTable)
    {
    }

    template<typename T>
    Task(void (*entryPoint)(Task* task, T* args), const T& args, const std::shared_ptr<PageTable>& pageTable)
    : Task(reinterpret_cast<EntryPoint>(entryPoint), &args, sizeof(args), pageTable)
    {
    }

    // TODO: how can we ensure only the scheduler is allowed to delete a Task?
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
    TaskState           m_state;                // Scheduling state
    TaskPriority        m_priority;             // Task priority
    WaitQueue*          m_queue;                // Where does this task live?
    Task*               m_next;                 // Next task in the TaskList
    Task*               m_prev;                 // Previous task in the TaskList

    TaskRegisters*      m_context;              // Saved context (on the task's stack)

    std::shared_ptr<PageTable> m_pageTable;     // Page table

    uint64_t            m_sleepUntilNs;         // Sleep until this time (clock time in nanoseconds)

    void*               GetKernelStackTop() const   { return (void*)(this + 1); }
    void*               GetKernelStack() const      { return (char*)this + STACK_PAGE_COUNT * MEMORY_PAGE_SIZE; }

    void*               m_userStackTop;         // Top of user stack
    void*               m_userStackBottom;      // Bottom of user stack
    void*               m_userTask;             // User task (if any) - void* to ensure the kernel doesn't use and rely on its fields

    // TODO: move IPC WaitQueue outside the TCB?
    WaitQueue           m_ipcSenders;           // List of tasks blocked on ipc_call
    // TODO: move IPC virtual registers out of TCB and map them in user space (UTCB, gs:0 in userspace)
    ipc_endpoint_t      m_ipcPartner;           // Who is our IPC partner?
    uintptr_t           m_ipcRegisters[64];     // Virtual registers for IPC

    FpuState            m_fpuState;             // FPU state

    // Return whether or not this task is blocked
    bool IsBlocked() const { return m_state >= TaskState::Sleep; }

    // Platform specific task-switching
    static void ArchSwitch(Task* currentTask, Task* newTask);

    // Wakeup this task
    void Wakeup();


private:

    // Platform specific initialization
    void ArchInit(EntryPoint entryPoint, const void* args);

    // Entry point for new tasks
    static void Entry(Task* task, EntryPoint entryPoint, const void* args) __attribute__((noreturn));
};


#endif
