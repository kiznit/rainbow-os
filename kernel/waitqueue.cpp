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
#include <kernel/task.hpp>


void WaitQueue::push_back(Task* task)
{
    assert(task->m_queue == nullptr);

    task->m_queue = this;
    m_tasks.push_back(task);
}


Task* WaitQueue::pop_front()
{
    assert(!m_tasks.empty());

    auto task = m_tasks.front();
    m_tasks.erase(m_tasks.begin());
    task->m_queue = nullptr;

    return task;
}


void WaitQueue::remove(Task* task)
{
    assert(task->m_queue == this);

    m_tasks.erase(std::find(m_tasks.begin(), m_tasks.end(), task));
    task->m_queue = nullptr;
}


bool WaitQueue::empty() const
{
    return m_tasks.empty();
}


Task* WaitQueue::front() const
{
    return m_tasks.empty() ? nullptr : m_tasks.front();
}
