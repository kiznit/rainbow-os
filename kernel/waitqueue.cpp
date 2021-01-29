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
#include <kernel/readyqueue.hpp>
#include <kernel/scheduler.hpp>
#include <kernel/task.hpp>

extern ReadyQueue g_readyQueue;


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

        m_tasks.emplace_back(task); // TODO: this could throw and task would be in invalid state / limbo

        assert(task->IsBlocked());
    }

    // if (nextTask == nullptr)
    // {
         sched_schedule();
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

    auto it = std::find_if(m_tasks.begin(), m_tasks.end(), [&task](auto& p) { return p.get() == task; });
    if (it != m_tasks.end())
    {
        // TODO: this function is not exception safe (Queue could throw)
        g_readyQueue.Queue(std::move(*it));
        m_tasks.erase(it);
        task->m_queue = nullptr;

        // TODO: maybe we want to prempt the current task and execute the unblocked one
    }
}


void WaitQueue::WakeupOne()
{
    std::lock_guard lock(m_lock);

    if (!m_tasks.empty())
    {
        auto task = std::move(m_tasks.front());
        m_tasks.erase(m_tasks.begin());

        task->m_queue = nullptr;
        // TODO: this function is not exception safe (Queue could throw)
        g_readyQueue.Queue(std::move(task));
    }
}


void WaitQueue::WakeupAll()
{
    std::lock_guard lock(m_lock);

    for (auto& task: m_tasks)
    {
        task->m_queue = nullptr;
        // TODO: this function is not exception safe (Queue could throw)
        g_readyQueue.Queue(std::move(task));
    }

    m_tasks.clear();
}


void WaitQueue::WakeupUntil(uint64_t timeNs)
{
    std::lock_guard lock(m_lock);

    for (auto it = m_tasks.begin(); it != m_tasks.end(); )
    {
        auto& task = *it;
        if (task->m_sleepUntilNs <= timeNs)
        {
            assert(task->m_state == TaskState::Sleep);

            task->m_queue = nullptr;
            g_readyQueue.Queue(std::move(task));
            it = m_tasks.erase(it);
        }
        else
        {
            ++it;
        }
    }
}


std::unique_ptr<Task> WaitQueue::PopBack()
{
    std::lock_guard lock(m_lock);

    if (m_tasks.empty())
    {
        return nullptr;
    }

    auto task = std::move(m_tasks.back());
    m_tasks.pop_back();

    return task;
}
