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
#include <kernel/kernel.hpp>


static const int STACK_PAGE_COUNT = 1;

static volatile Task::Id s_nextTaskId = 0;

// TODO: this is temporary until we have a proper associative structure (hashmap?)
static Task* s_tasks[100];



Task* Task::Get(Id id)
{
    if (id < (int)ARRAY_LENGTH(s_tasks))
        return s_tasks[id];
    else
        return nullptr;
}



Task* Task::InitTask0()
{
    extern const char _boot_stack_top[];
    extern const char _boot_stack[];

    auto task = (Task*)(_boot_stack - STACK_PAGE_COUNT * MEMORY_PAGE_SIZE);
    task->id = 0;
    task->state = STATE_RUNNING;

    task->kernelStackTop = (char*)(task + 1);
    task->kernelStackBottom = (char*)task + STACK_PAGE_COUNT * MEMORY_PAGE_SIZE;

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



Task* Task::Create(EntryPoint entryPoint, const void* args, int flags)
{
    // Allocate
    auto task = (Task*)vmm_allocate_pages(STACK_PAGE_COUNT);
    if (!task) return nullptr; // TODO: we should probably do better

    // Initialize
    memset(task, 0, sizeof(*task));
    task->id = __sync_add_and_fetch(&s_nextTaskId, 1);
    task->state = STATE_INIT;

    task->kernelStackTop = (char*)(task + 1);
    task->kernelStackBottom = (char*)task + STACK_PAGE_COUNT * MEMORY_PAGE_SIZE;

    task->pageTable = cpu_get_data(task)->pageTable;

    if (!(flags & CREATE_SHARE_PAGE_TABLE))
    {
        if (!task->pageTable.CloneKernelSpace())
        {
            // TODO: we should probably do better
            vmm_free_pages(task, 1);
            return nullptr;
        }
    }

    assert(task->id < (int)ARRAY_LENGTH(s_tasks));
    s_tasks[task->id] = task;

    if (!Initialize(task, entryPoint, args))
    {
        // TODO: we should probably do better
        // TODO: we need to free the page table if it was cloned above
        vmm_free_pages(task, 1);
        return nullptr;
    }

    // Schedule the task
    task->state = STATE_READY;
    sched_add_task(task);

    return task;
}



void Task::Entry()
{
    //Task* task = cpu_get_data(task);
    //Log("Task::Entry(), id %d\n", task->id);
}



void Task::Exit()
{
    Task* task = cpu_get_data(task);

    Log("Task::Exit(), id %d\n", task->id);


    //todo: kill current task (i.e. zombify it)
    //todo: remove task from scheduler
    //todo: yield() / schedule()
    //todo: free the kernel stack
    //todo: free the task

    //todo
    //cpu_halt();
    for (;;);
}
