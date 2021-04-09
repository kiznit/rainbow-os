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

#include "waitqueue.hpp"

#include <cassert>
#include <mutex>

#include "kernel.hpp"
#include "readyqueue.hpp"
#include "scheduler.hpp"
#include "task.hpp"

extern Scheduler g_scheduler;


WaitQueue::~WaitQueue()
{
    WakeupAll();
}


void WaitQueue::Suspend(TaskState reason/*, Task* nextTask*/)
{
    auto task = cpu_get_data(task);

    //assert(task != nextTask);
    assert(task->m_state == TaskState::Running);
    assert(task->m_queue == nullptr);

    // Scope for the lock guard
    {
        std::lock_guard lock(m_lock);

        task->m_queue = this;
        task->m_state = reason;

        m_tasks.push_back(task);

        assert(task->IsBlocked());
    }

    // if (nextTask == nullptr)
    // {
         g_scheduler.Schedule();
    // }
    // else
    // {
    //     // TODO: I am not sure I want this functionality (nextTask), better have the scheduler figure it out?
    //     sched_switch(nextTask);
    // }
}


void WaitQueue::Wakeup(Task* task)
{
    assert(task->IsBlocked());
    assert(task->m_queue == this);

    std::lock_guard lock(m_lock);

    assert(task->m_queue == this);

    m_tasks.remove(task);
    task->m_queue = nullptr;

    g_scheduler.AddTask(std::unique_ptr<Task>(task));

    // TODO: maybe we want to prempt the current task and execute the unblocked one
}


int WaitQueue::Wakeup(int count)
{
    int released = 0;

    std::lock_guard lock(m_lock);

    for ( ; !m_tasks.empty() && count > 0; --count)
    {
        auto task = std::unique_ptr<Task>(m_tasks.pop_front());
        task->m_queue = nullptr;

        g_scheduler.AddTask(std::move(task));

        ++released;
    }

    return released;
}


int WaitQueue::WakeupAll()
{
    int released = 0;

    std::lock_guard lock(m_lock);
    while (!m_tasks.empty())
    {
        auto task = std::unique_ptr<Task>(m_tasks.pop_front());
        task->m_queue = nullptr;

        g_scheduler.AddTask(std::move(task));

        ++released;
    }

    return released;
}


void WaitQueue::WakeupUntil(uint64_t timeNs)
{
    std::lock_guard lock(m_lock);

    for (Task* task = m_tasks.front(); task != nullptr; )
    {
        if (task->m_sleepUntilNs <= timeNs)
        {
            assert(task->m_state == TaskState::Sleep);

            auto nextTask = m_tasks.remove(task);
            task->m_queue = nullptr;

            g_scheduler.AddTask(std::unique_ptr<Task>(task));

            task = nextTask;
        }
        else
        {
            task = task->m_next;
        }
    }
}


std::unique_ptr<Task> WaitQueue::PopFront()
{
    std::lock_guard lock(m_lock);

    if (m_tasks.empty())
    {
        return nullptr;
    }

    auto task = std::unique_ptr<Task>(m_tasks.pop_front());
    task->m_queue = nullptr;

    return task;
}
