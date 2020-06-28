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

#ifndef _RAINBOW_KERNEL_WAITQUEUE_INL
#define _RAINBOW_KERNEL_WAITQUEUE_INL


inline void WaitQueue::push_back(Task* task)
{
    //Log("Push %d on queue %p\n", task->id, this);
    assert(task->queue == nullptr);
    assert(task->next == nullptr);
    task->queue = this;
    m_tasks.push_back(task);
}


inline Task* WaitQueue::pop_front()
{
    auto task = m_tasks.pop_front();
    //Log("Pop %d on queue %p\n", task->id, this);
    assert(task != nullptr);
    assert(task->next == nullptr);
    task->queue = nullptr;
    return task;
}


inline void WaitQueue::remove(Task* task)
{
    //Log("Remove %d on queue %p\n", task->id, this);
    assert(task->queue == this);
    m_tasks.remove(task);
    assert(task->next == nullptr);
    task->queue = nullptr;
}


inline bool WaitQueue::empty() const
{
    return m_tasks.empty();
}


inline Task* WaitQueue::front() const
{
    return m_tasks.front();
}


#endif
