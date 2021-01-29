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

#include "mutex.hpp"
#include <climits>
#include <kernel/scheduler.hpp>
#include <kernel/task.hpp>

extern bool g_isEarly;



Mutex::Mutex()
:   m_owner(-1)
{
}


void Mutex::lock()
{
    while (!try_lock())
    {
        // TODO: we are suffering from the lost wake-up problem here!
        m_waiters.Suspend(TaskState::Mutex);
    }
}


bool Mutex::try_lock()
{
    assert(!g_isEarly);

    if (!m_lock.exchange(true, std::memory_order_acquire))
    {
        m_owner = cpu_get_data(task)->m_id;
        return true;
    }
    else
    {
        return false;
    }
}


void Mutex::unlock()
{
    assert(m_owner == cpu_get_data(task)->m_id);

    m_owner = -1;

    m_lock.store(false, std::memory_order_release);

    m_waiters.WakeupOne();
}



RecursiveMutex::RecursiveMutex()
:   m_owner(-1),
    m_count(0)
{
}


void RecursiveMutex::lock()
{
    assert(!g_isEarly);

    while (!try_lock())
    {
        m_waiters.Suspend(TaskState::Mutex);
    }
}


bool RecursiveMutex::try_lock()
{
    const auto taskId = cpu_get_data(task)->m_id;

    if (!m_lock.exchange(true, std::memory_order_acquire))
    {
        m_owner = taskId;
        m_count = 1;
        return true;
    }
    else if (m_owner == taskId && m_count < INT_MAX)
    {
        ++m_count;
        return true;
    }
    else
    {
        // TODO: if we failed because m_count == INT_MAX, we might want to throw an exception
        return false;
    }
}


void RecursiveMutex::unlock()
{
    const auto taskId = cpu_get_data(task)->m_id;

    assert(m_owner == taskId);
    assert(m_count > 0);

    if (--m_count == 0)
    {
        m_owner = -1;
        m_lock.store(false, std::memory_order_release);
    }

    m_waiters.WakeupOne();
}
