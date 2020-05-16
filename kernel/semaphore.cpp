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

#include "semaphore.hpp"
#include <metal/crt.hpp>
#include <metal/log.hpp>
#include "kernel.hpp"
#include "task.hpp"


Semaphore::Semaphore(int initialCount)
:   m_count(initialCount),
    m_firstWaiter(nullptr),
    m_lastWaiter(nullptr)
{
    assert(initialCount > 0);
}


void Semaphore::Lock()
{
    g_scheduler->Lock();

    //Log("Lock(%d)\n", cpu_get_data(task)->id);

    if (m_count > 0)
    {
        // Lock acquired
        --m_count;
    }
    else
    {
        // Blocked - queue current task and yield
        auto task = cpu_get_data(task);

        //Log("Lock(%d) - blocking task\n", task->id);

        if (m_firstWaiter == nullptr)
        {
            m_lastWaiter = m_firstWaiter = task;
        }
        else
        {
            m_lastWaiter = m_lastWaiter->next = task;
        }

        g_scheduler->Suspend();

        //Log("Back from suspend(%d)", task->id);
    }

    g_scheduler->Unlock();
}


int Semaphore::TryLock()
{
    g_scheduler->Lock();

    if (m_count > 0)
    {
        // Lock acquired
        --m_count;
        g_scheduler->Unlock();
        return 1;
    }
    else
    {
        // Failed to lock
        g_scheduler->Unlock();
        return 0;
    }
}


void Semaphore::Unlock()
{
    g_scheduler->Lock();

    //Log("Unlock(%d)\n", cpu_get_data(task)->id);

    if (m_firstWaiter == nullptr)
    {
        // No task waiting, increment counter
        ++m_count;
    }
    else
    {
        // Wake up the oldest blocked task (first waiter)
        auto task = m_firstWaiter;
        m_firstWaiter = task->next;
        if (m_firstWaiter == nullptr)
        {
            m_lastWaiter = nullptr;
        }

        task->next = nullptr;

        g_scheduler->Wakeup(task);
    }

    g_scheduler->Unlock();
}
