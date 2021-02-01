/*
    Copyright (c) 2021, Thierry Tremblay
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

#include "readyqueue.hpp"
#include <cassert>
#include <mutex>
#include <kernel/task.hpp>


void ReadyQueue::Queue(std::unique_ptr<Task>&& task)
{
    std::lock_guard lock(m_lock);

    task->m_state = TaskState::Ready;
    m_tasks[static_cast<int>(task->m_priority)].push_back(std::move(task));
}


std::unique_ptr<Task> ReadyQueue::Pop()
{
    std::lock_guard lock(m_lock);

    // Find a task to run, start with higher priorities
    // TODO: need some fairness here to prevent threads from starving lower priority threads
    for (auto i = TaskPriorityCount - 1; i >= 0; --i)
    {
        auto& subqueue = m_tasks[i];

        if (!subqueue.empty())
        {
            auto task = std::move(subqueue.front());
            subqueue.erase(subqueue.begin());
            return task;
        }
    }

    return nullptr;
}
