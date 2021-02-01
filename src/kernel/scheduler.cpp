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

#include "scheduler.hpp"
#include <cassert>
#include <cstring>
#include <kernel/kernel.hpp>
#include <kernel/vmm.hpp>
#include <metal/log.hpp>

//#define TRACE(...) Log(__VA_ARGS__)
#define TRACE(...)

struct InterruptContext;

extern ITimer* g_timer;
extern std::shared_ptr<PageTable> g_kernelPageTable;


bool sched_should_switch;                       // Should we switch task?


static int TimerCallback(InterruptContext*)
{
    sched_should_switch = true;
    return 1;
}


static Task* init_task0()
{
    extern const char _boot_stack_top[];
    extern const char _boot_stack[];

    const auto bootStackSize = _boot_stack - _boot_stack_top;
    const auto kernelStackSize = STACK_PAGE_COUNT * MEMORY_PAGE_SIZE;
    assert(kernelStackSize <= bootStackSize);

    auto memory = (void*)(_boot_stack - kernelStackSize);

#pragma GCC diagnostic ignored "-Warray-bounds"
    memset(memory, 0, sizeof(Task));
#pragma GCC diagnostic error "-Warray-bounds"

    auto task0 = new (memory) Task(g_kernelPageTable);
    assert(task0->m_id == 0);

    task0->m_state = TaskState::Running;

    // Free boot stack
    auto pagesToFree = ((char*)task0 - _boot_stack_top) >> MEMORY_PAGE_SHIFT;
    vmm_free_pages((void*)_boot_stack_top, pagesToFree);

    return task0;
}


void Scheduler::Initialize()
{
    assert(!interrupt_enabled());

    auto task0 = init_task0();

    cpu_set_data(task, task0);

    g_timer->Initialize(200, TimerCallback);    // 200 Hz = 5ms
}


void Scheduler::AddTask(std::unique_ptr<Task>&& task)
{
    assert(!interrupt_enabled());

    TRACE("%d: Scheduler::AddTask(): task id %d\n", cpu_get_data(task)->m_id, task->m_id);

    m_ready.Queue(std::move(task));
}


// TODO: make sure "newTask" is not in a ReadyQueue, as this function
// will not remove it... maybe I want to change that.
void Scheduler::Switch(std::unique_ptr<Task>&& newTask)
{
    assert(!interrupt_enabled());
    assert(newTask->m_state == TaskState::Ready);

    auto currentTask = cpu_get_data(task);
    TRACE("%d: Scheduler::Switch() to task %d in state %d\n", currentTask->m_id, newTask->m_id, newTask->m_state);
    assert(newTask.get() != currentTask);

    if (currentTask->m_state == TaskState::Running)
    {
        TRACE("%d: Scheduler::Switch - task %d still running, putting in ready queue\n", currentTask->m_id, currentTask->m_id);
        m_ready.Queue(std::unique_ptr<Task>(currentTask));
    }
    else
    {
        // It is assumed that currentTask is queued in the appropriate WaitQueue somewhere
        assert(currentTask->IsBlocked() || currentTask->m_state == TaskState::Ready);
        assert(currentTask->m_queue != nullptr);
    }

    newTask->m_state = TaskState::Running;

    // Make sure we can't be interrupted between the next two statement, otherwise state will be inconsistent
    assert(!interrupt_enabled());
    cpu_set_data(task, newTask.get());
    Task::ArchSwitch(currentTask, newTask.release());
}


void Scheduler::Schedule()
{
    auto currentTask = cpu_get_data(task);
    TRACE("%d: Scheduler::Schedule()\n", currentTask->m_id);

    assert(!interrupt_enabled());

    assert(currentTask->m_state == TaskState::Running || currentTask->IsBlocked());

    // Find a task to run
    std::unique_ptr<Task> newTask = m_ready.Pop();
    if (newTask)
    {
        TRACE("%d: Scheduler::Schedule() selected task %d\n", currentTask->m_id, newTask->m_id);
    }
    else
    {
        TRACE("%d: Scheduler::Schedule() no task to run, we are in state %d\n", currentTask->m_id, currentTask->m_state);
    }

    // Any other task to run?
    if (newTask)
    {
        Switch(std::move(newTask));
    }
    else
    {
        // TODO: properly handle case where the current task is blocked (use idle task or idle loop)
        //Log("Schedule() - Ready list is empty, current task will continue to run (state %d)\n", currentTask->m_state);
        assert(currentTask->m_state == TaskState::Running);
    }

    // Destroy any zombie
    // TODO: is this the right place? do we want to use a cleanup task to handle zombies?
    if (currentTask->m_state != TaskState::Zombie) // TODO: hacks because current task might be a zombie!
    {
        std::unique_ptr<Task> zombie;
        while ((zombie = m_zombies.PopBack()))
        {
            // Nothing
        }
    }

    // Wakeup any sleeping tasks
    // Note: waking up sleeping tasks NEEDS to be done after we call Switch(). The reason for this
    // is that if the running task is put to sleep for a small duration, it can be awoken by WakeupUntil()
    // that will put the task back in ReadyQueue. When this happens, invariants are broken everywhere.
    // I've seen this happen with Bochs ia32 because emulation is slow. But it could happen in real life.
    // TODO: the design probably needs fixing
    m_sleeping.WakeupUntil(g_clock->GetTimeNs());
}


void Scheduler::Sleep(uint64_t durationNs)
{
    SleepUntil(g_clock->GetTimeNs() + durationNs);
}


void Scheduler::SleepUntil(uint64_t clockTimeNs)
{
    auto task = cpu_get_data(task);

    TRACE("%d: Scheduler::SleepUntil()\n", task->m_id);

    task->m_sleepUntilNs = clockTimeNs;

    // TODO: here we might want to setup a timer to ensure the kernel is
    // entered and the task woken up when we reach "clockTimeNs".

    m_sleeping.Suspend(TaskState::Sleep);
}


void Scheduler::Yield()
{
    assert(!interrupt_enabled());

    sched_should_switch = true;
    Schedule();
}


void Scheduler::Die(int status)
{
    // TODO: use status
    (void)status;

    m_zombies.Suspend(TaskState::Zombie);

    // Should never be reached
    for (;;);
}
