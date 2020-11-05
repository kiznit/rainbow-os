
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

#include "task.hpp"
#include <kernel/biglock.hpp>
#include <kernel/kernel.hpp>


// TODO: should not be visible outside
/*static*/ volatile Task::Id s_nextTaskId = 0;

// TODO: this is temporary until we have a proper associative structure (hashmap?)
static Task* s_tasks[100];


Task* Task::Allocate()
{
    auto task = (Task*)vmm_allocate_pages(STACK_PAGE_COUNT);    // TODO: error handling
    memset(task, 0, sizeof(*task));                             // TODO: vmm_allocate_pages should return zeroed pages?

    // Allocate a task id
    task->id = __sync_add_and_fetch(&s_nextTaskId, 1);

    // Set initial state
    task->state = STATE_INIT;

    //Log("Task %d allocated\n", task->id);

    return task;
}


void Task::Free(Task* task)
{
    s_tasks[task->id] = nullptr;
    vmm_free_pages(task, 1);
}


Task* Task::Get(Id id)
{
    if (id < (int)ARRAY_LENGTH(s_tasks))
        return s_tasks[id];
    else
        return nullptr;
}


void Task::Idle()
{
    for (;;)
    {
        assert(g_bigKernelLock.IsLocked());

        // TEMP: if there is any task to run, do not go idle
        // TODO: need better handling here, ideally the idle task doesn't get to run at all
        if (sched_pending_work())
        {
            sched_schedule();
        }
        else
        {
            g_bigKernelLock.Unlock();
            interrupt_enable();

            x86_halt();

            interrupt_disable();
            g_bigKernelLock.Lock();
        }
    }
}


Task* Task::InitTask0()
{
    extern const char _boot_stack_top[];
    extern const char _boot_stack[];

    const auto bootStackSize = _boot_stack - _boot_stack_top;
    const auto kernelStackSize = STACK_PAGE_COUNT * MEMORY_PAGE_SIZE;
    assert(kernelStackSize <= bootStackSize);

    auto task = (Task*)(_boot_stack - kernelStackSize);
    task->id = 0;
    task->state = STATE_RUNNING;

    // Task zero has no user space
    task->userStackTop = 0;
    task->userStackBottom = 0;

    task->pageTable.cr3 = x86_get_cr3();      // TODO: platform specific code does not belong here

    s_tasks[0] = task;

    // Free boot stack
    auto pagesToFree = ((char*)task - _boot_stack_top) >> MEMORY_PAGE_SHIFT;
    vmm_free_pages((void*)_boot_stack_top, pagesToFree);

    return task;
}


Task* Task::CreateImpl(EntryPoint entryPoint, int flags, const void* args, size_t sizeArgs)
{
    // Allocate
    auto task = Allocate();

    // Initialize
    task->pageTable = cpu_get_data(task)->pageTable;

    if (!(flags & CREATE_SHARE_PAGE_TABLE))
    {
        if (!task->pageTable.CloneKernelSpace())
        {
            // TODO: we should probably do better
            Free(task);
            return nullptr;
        }
    }

    assert(task->id < (int)ARRAY_LENGTH(s_tasks));
    s_tasks[task->id] = task;

    // If args is an object, we want to copy it somewhere inside the new thread's context.
    // The top of the stack works just fine (for now?).
    if (sizeArgs > 0)
    {
        memcpy(task + 1, args, sizeArgs);
        args = task + 1;
    }

    if (!Initialize(task, entryPoint, args))
    {
        // TODO: we should probably do better
        // TODO: we need to free the page table if it was cloned above
        Free(task);
        return nullptr;
    }

    // Schedule the task
    task->state = STATE_READY;
    sched_add_task(task);

    return task;
}


void Task::Entry(Task* task, EntryPoint entryPoint, const void* args)
{
    assert(!interrupt_enabled());
    g_bigKernelLock.Lock();

    //Log("Task::Entry(), id %d, entryPoint %p, args %p\n", task->id, entryPoint, args);

    entryPoint(task, args);

    Log("Task %d exiting\n", task->id);

    //todo: kill current task (i.e. zombify it)
    //todo: remove task from scheduler
    //todo: yield() / schedule()
    //todo: free the kernel stack
    //todo: free the task

    for (;;)
    {
        sched_schedule();
    }
}
