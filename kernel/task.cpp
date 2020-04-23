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


static volatile Task::Id s_nextTaskId = 0;


static Task s_task0;        // Initial kernel task

// TODO: this is temporary until we have a proper associative structure (hashmap?)
static Task* s_tasks[100];



Task* Task::Get(Id id)
{
    if (id < ARRAY_LENGTH(s_tasks))
        return s_tasks[id];
    else
        return nullptr;
}



Task* Task::InitTask0()
{
    Task* task = &s_task0;

    task->id = 0;
    task->state = STATE_RUNNING;
    task->context = nullptr;
    task->pageTable.cr3 = x86_get_cr3();      // TODO: platform specific code does not belong here

    // TODO
    task->kernelStackTop = 0;
    task->kernelStackBottom = 0;

    // Task zero has no user space
    task->userStackTop = 0;
    task->userStackBottom = 0;

    task->next = nullptr;

    s_tasks[0] = task;

    return task;
}



Task* Task::Create(EntryPoint entryPoint, const void* args, int flags)
{
    // Allocate
    auto task = new Task();
    if (!task) return nullptr; // TODO: we should probably do better

    // Initialize
    memset(task, 0, sizeof(*task));
    task->id = __sync_add_and_fetch(&s_nextTaskId, 1);
    task->state = STATE_INIT;
    task->pageTable = g_scheduler->GetCurrentTask()->pageTable;

    if (!(flags & CREATE_SHARE_PAGE_TABLE))
    {
        if (!task->pageTable.CloneKernelSpace())
        {
            // TODO: we should probably do better
            delete task;
            return nullptr;
        }
    }

    assert(task->id < ARRAY_LENGTH(s_tasks));
    s_tasks[task->id] = task;

    if (!Initialize(task, entryPoint, args))
    {
        // TODO: we should probably do better
        // TODO: we need to free the page table if it was cloned above
        delete task;
        return nullptr;
    }

    // Schedule the task
    g_scheduler->Lock();
    task->state = STATE_READY;
    g_scheduler->AddTask(task);
    g_scheduler->Unlock();

    return task;
}



void Task::Entry()
{
    Task* task = g_scheduler->GetCurrentTask();

    Log("Task::Entry(), id %d\n", task->id);

    // We got here immediately after a call to Scheduler::Switch().
    // This means we still have the scheduler lock and we must release it.
    g_scheduler->Unlock();
}



void Task::Exit()
{
    Task* task = g_scheduler->GetCurrentTask();

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
