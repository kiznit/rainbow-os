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
#include <metal/log.hpp>
#include <kernel/kernel.hpp>
#include <kernel/vmm.hpp>


struct InterruptContext;

extern ITimer* g_timer;
extern std::shared_ptr<PageTable> g_kernelPageTable;


static WaitQueue s_ready[Task::PRIORITY_COUNT]; // List of ready tasks
static WaitQueue s_sleeping;                    // Sleeping tasks - TODO: keep sorted?
static WaitQueue s_zombies;                     // Task is dying

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

    task0->m_state = Task::STATE_RUNNING;

    // Free boot stack
    auto pagesToFree = ((char*)task0 - _boot_stack_top) >> MEMORY_PAGE_SHIFT;
    vmm_free_pages((void*)_boot_stack_top, pagesToFree);

    return task0;
}


void sched_initialize()
{
    assert(!interrupt_enabled());

    auto task0 = init_task0();

    cpu_set_data(task, task0);

    g_timer->Initialize(200, TimerCallback);    // 200 Hz = 5ms
}


void sched_add_task(Task* task)
{
    assert(!interrupt_enabled());

    assert(task->m_state == Task::STATE_INIT);

    task->m_state = Task::STATE_READY;
    s_ready[task->m_priority].push_back(task);
}


void sched_switch(Task* newTask)
{
    assert(!interrupt_enabled());
    assert(newTask->m_state == Task::STATE_READY);

    auto currentTask = cpu_get_data(task);
    //Log("%d: Switch() to task %d in state %d\n", currentTask->m_id, newTask->m_id, newTask->m_state);

    if (currentTask == newTask)
    {
        // If the current task isn't running, we might have a problem?
        assert(currentTask->m_state == Task::STATE_RUNNING);
        Log("%d: same task, keep running...\n", currentTask->m_id);
        return;
    }

    assert(!newTask->IsBlocked());
    assert(newTask->m_queue);
    newTask->m_queue->remove(newTask);

    if (currentTask->m_state == Task::STATE_RUNNING)
    {
        //Log("Switch - task %d still running\n", currentTask->m_id);
        currentTask->m_state = Task::STATE_READY;
        s_ready[currentTask->m_priority].push_back(currentTask);
    }
    else
    {
        // It is assumed that currentTask is queued in the appropriate WaitQueue somewhere
        assert(currentTask->IsBlocked() || currentTask->m_state == Task::STATE_READY);
        assert(currentTask->m_queue != nullptr);
    }

    newTask->m_state = Task::STATE_RUNNING;

    // Make sure we can't be interrupted between the next two statement, otherwise state will be inconsistent
    assert(!interrupt_enabled());

    cpu_set_data(task, newTask);
    Task::ArchSwitch(currentTask, newTask);
}


void sched_schedule()
{
    assert(!interrupt_enabled());

    auto currentTask = cpu_get_data(task);

    assert(currentTask->m_state == Task::STATE_RUNNING || currentTask->IsBlocked());

    // Destroy any zombie
    // TODO: is this the right place? do we want to use a cleanup task to handle zombies?
    if (currentTask->m_state != Task::STATE_ZOMBIE) // TODO: hacks because current task might be a zombie!
    {
        while (!s_zombies.empty())
        {
            delete s_zombies.pop_front();
        }
    }

    // Wakeup any sleeping tasks
    // TODO: is this the right place / logic? Doesn't seem optimal...
    if (!s_sleeping.empty())
    {
        const auto now = g_clock->GetTimeNs();

        Task* task;
        while ((task = s_sleeping.find_sleeping(now)) != nullptr)
        {
            assert(task->m_state == Task::STATE_SLEEP);
            s_sleeping.remove(task);

            task->m_state = Task::STATE_READY;
            // TODO: we might want to prioritize newly awoken tasks
            s_ready[task->m_priority].push_back(task);
        }
    }

    // Find a task to run, start with higher priorities
    // TODO: need some fairness here to prevent threads from starving lower priority threads
    const WaitQueue* readyQueue = nullptr;

    for (auto i = Task::PRIORITY_COUNT - 1; i >= 0; --i)
    {
        if (!s_ready[i].empty())
        {
            readyQueue = &s_ready[i];
            break;
        }
    }

    // Any other task to run?
    if (!readyQueue)
    {
        // TODO: properly handle case where the current task is blocked (use idle task or idle loop)
        //Log("Schedule() - Ready list is empty, current task will continue to run (state %d)\n", currentTask->m_state);
        assert(currentTask->m_state == Task::STATE_RUNNING);
        return;
    }

    // Do not switch to a lower priority task for now
    if (currentTask->m_state == Task::STATE_RUNNING && readyQueue->front()->m_priority < currentTask->m_priority)
    {
        assert(currentTask->m_state == Task::STATE_RUNNING);
        return;
    }

    sched_switch(readyQueue->front());
}


void sched_suspend(WaitQueue& queue, Task::State reason, Task* nextTask)
{
    assert(!interrupt_enabled());

    auto task = cpu_get_data(task);
    assert(task != nextTask);

    //Log("%d: Suspend() reason: %d\n", task->m_id, reason);

    assert(task->m_state == Task::STATE_RUNNING);
    assert(task->m_queue == nullptr);

    task->m_state = reason;
    queue.push_back(task);

    assert(task->IsBlocked());
    assert(task->m_queue != nullptr);

    if (nextTask != nullptr)
    {
        sched_switch(nextTask);
    }
    else
    {
        sched_schedule();
    }
}


void sched_wakeup(Task* task)
{
    assert(!interrupt_enabled());

    //Log("%d: Wakeup() task %d, state %d\n", cpu_get_data(task)->id, task->m_id, task->m_state);

    assert(task != cpu_get_data(task));

    assert(task->IsBlocked());
    assert(task->m_queue != nullptr);

    task->m_queue->remove(task);
    assert(task->m_queue == nullptr);

    // TODO: maybe we want to prempt the current task and execute the unblocked one
    task->m_state = Task::STATE_READY;
    s_ready[task->m_priority].push_back(task);

    assert(task->m_queue);
}


void sched_sleep(uint64_t durationNs)
{
    sched_sleep_until(g_clock->GetTimeNs() + durationNs);
}


void sched_sleep_until(uint64_t clockTimeNs)
{
    auto task = cpu_get_data(task);

    task->m_sleepUntilNs = clockTimeNs;

    // TODO: here we might want to setup a timer to ensure the kernel is
    // entered and the task woken up when we reach "clockTimeNs".

    sched_suspend(s_sleeping, Task::STATE_SLEEP);
}


void sched_yield()
{
    assert(!interrupt_enabled());

    sched_should_switch = true;
    sched_schedule();
}


void sched_die()
{
    sched_suspend(s_zombies, Task::STATE_ZOMBIE);
    for (;;);
}


bool sched_pending_work()
{
    for (auto i = 0; i != Task::PRIORITY_COUNT; ++i)
    {
        if (!s_ready[i].empty())
        {
            return true;
        }
    }

    return false;
}
