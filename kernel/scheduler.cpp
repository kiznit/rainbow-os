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
#include <metal/log.hpp>
#include "spinlock.hpp"
#include "timer.hpp"
#include "waitqueue.inl"


struct InterruptContext;

extern ITimer* g_timer;

static WaitQueue s_ready;   // List of ready tasks - TODO: should this be a "WaitQueue"?

bool sched_should_switch;   // Should we switch task?


static int TimerCallback(InterruptContext*)
{
    sched_should_switch = true;
    return 1;
}


void sched_initialize()
{
    assert(!interrupt_enabled());

    auto task0 = Task::InitTask0();
    cpu_set_data(task, task0);

    g_timer->Initialize(1, TimerCallback);
}


void sched_add_task(Task* task)
{
    assert(!interrupt_enabled());

    assert(task->state == Task::STATE_READY);
    s_ready.push_back(task);
}


void sched_switch(Task* newTask)
{
    assert(!interrupt_enabled());
    assert(newTask->state == Task::STATE_READY);

    auto currentTask = cpu_get_data(task);
    //Log("%d: Switch() to task %d in state %d\n", currentTask->id, newTask->id, newTask->state);

    if (currentTask == newTask)
    {
        // If the current task isn't running, we might have a problem?
        assert(currentTask->state == Task::STATE_RUNNING);
        Log("%d: same task, keep running...\n", currentTask->id);
        return;
    }

    // TODO: right now we only have a "ready" list, but eventually we will need to remove the task from the right list
    assert(!newTask->IsBlocked());
    assert(newTask->queue == &s_ready);
    s_ready.remove(newTask);

    if (currentTask->state == Task::STATE_RUNNING)
    {
        //Log("Switch - task %d still running\n", currentTask->id);
        currentTask->state = Task::STATE_READY;
        s_ready.push_back(currentTask);
    }
    else
    {
        // It is assumed that currentTask is queued in the appropriate WaitQueue somewhere
        assert(currentTask->IsBlocked());
        assert(currentTask->queue != nullptr);
    }

    newTask->state = Task::STATE_RUNNING;

    // Make sure we can't be interrupted between the next two statement, otherwise state will be inconsistent
    assert(!interrupt_enabled());

    cpu_set_data(task, newTask);
    Task::Switch(currentTask, newTask);
}


void sched_schedule()
{
    assert(!interrupt_enabled());

    auto currentTask = cpu_get_data(task);

    assert(currentTask->state == Task::STATE_RUNNING || currentTask->IsBlocked());

    if (s_ready.empty())
    {
        // TODO: properly handle case where the current thread is blocked (use idle thread or idle loop)
        //Log("Schedule() - Ready list is empty, current task will continue to run (state %d)\n", currentTask->state);
        assert(currentTask->state == Task::STATE_RUNNING);
        return;
    }

    sched_switch(s_ready.front());
}


void sched_suspend(WaitQueue& queue, Task::State reason, Task* nextTask)
{
    assert(!interrupt_enabled());

    auto task = cpu_get_data(task);
    assert(task != nextTask);

    //Log("%d: Suspend() reason: %d\n", task->id, reason);

    assert(task->state == Task::STATE_RUNNING);
    assert(task->queue == nullptr);
    assert(task->next == nullptr);

    task->state = reason;
    queue.push_back(task);

    assert(task->IsBlocked());
    assert(task->queue != nullptr);

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

    //Log("%d: Wakeup() task %d, state %d\n", cpu_get_data(task)->id, task->id, task->state);

    assert(task != cpu_get_data(task));

    assert(task->IsBlocked());
    assert(task->queue != nullptr);

    task->queue->remove(task);
    assert(task->queue == nullptr);
    assert(task->next == nullptr);

    // TODO: maybe we want to prempt the current task and execute the unblocked one
    task->state = Task::STATE_READY;
    s_ready.push_back(task);

    assert(task->queue == &s_ready);
    assert(task->next == nullptr);
}


void sched_yield()
{
    assert(!interrupt_enabled());

    sched_should_switch = true;
    sched_schedule();
}
