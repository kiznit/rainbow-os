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
#include <kernel/kernel.hpp>
#include <kernel/x86/pic.hpp>
#include "waitqueue.inl"


Scheduler::Scheduler()
:   m_lockCount(0),
    m_enableInterrupts(false),
    m_switch(false)
{
}


void Scheduler::Init()
{
    auto task0 = Task::InitTask0();
    cpu_set_data(task, task0);

    g_timer->Initialize(1, TimerCallback);
}


void Scheduler::Lock()
{
    m_enableInterrupts = interrupt_enabled();
    interrupt_disable();

    ++m_lockCount;
}


void Scheduler::Unlock()
{
    if (--m_lockCount == 0)
    {
        if (m_enableInterrupts)
        {
            interrupt_enable();
        }
    }
}


void Scheduler::AddTask(Task* task)
{
    assert(m_lockCount > 0);

    assert(task->state == Task::STATE_READY);
    m_ready.push_back(task);
}


void Scheduler::Switch(Task* newTask)
{
    assert(m_lockCount > 0);

    auto currentTask = cpu_get_data(task);

    //Log("%d: Switch() to task %d in state %d\n", currentTask->id, newTask->id, newTask->state);

    assert(!interrupt_enabled());
    assert(newTask->state == Task::STATE_READY);

    if (currentTask == newTask)
    {
        // If the current task isn't running, we might have a problem?
        assert(currentTask->state == Task::STATE_RUNNING);
        Log("%d: same task, keep running...\n", currentTask->id);
        return;
    }

    // TODO: right now we only have a "ready" list, but eventually we will need to remove the task from the right list
    assert(!newTask->IsBlocked());
    assert(newTask->queue == &m_ready);
    m_ready.remove(newTask);

    if (currentTask->state == Task::STATE_RUNNING)
    {
        //Log("Switch - task %d still running\n", currentTask->id);
        currentTask->state = Task::STATE_READY;
        m_ready.push_back(currentTask);
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


void Scheduler::Schedule()
{
    assert(m_lockCount > 0);

    auto currentTask = cpu_get_data(task);

    assert(currentTask->state == Task::STATE_RUNNING || currentTask->IsBlocked());

    assert(!interrupt_enabled());

    if (m_ready.empty())
    {
        // TODO: properly handle case where the current thread is blocked (use idle thread or idle loop)
        //Log("Schedule() - Ready list is empty, current task will continue to run (state %d)\n", currentTask->state);
        assert(currentTask->state == Task::STATE_RUNNING);
        return;
    }

    Switch(m_ready.front());
}


void Scheduler::Suspend(WaitQueue& queue, Task::State reason, Task* nextTask)
{
    assert(m_lockCount > 0);

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
        Switch(nextTask);
    }
    else
    {
        Schedule();
    }
}


void Scheduler::Wakeup(Task* task)
{
    assert(m_lockCount > 0);

    //Log("%d: Wakeup() task %d, state %d\n", cpu_get_data(task)->id, task->id, task->state);

    assert(task != cpu_get_data(task));

    assert(task->IsBlocked());
    assert(task->queue != nullptr);

    task->queue->remove(task);
    assert(task->queue == nullptr);
    assert(task->next == nullptr);

    // TODO: maybe we want to prempt the current task and execute the unblocked one
    task->state = Task::STATE_READY;
    m_ready.push_back(task);

    assert(task->queue == &m_ready);
    assert(task->next == nullptr);
}


int Scheduler::TimerCallback(InterruptContext*)
{
    g_scheduler->m_switch = true;

    return 1;
}
