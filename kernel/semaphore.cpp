/*
    Copyright (c) 2018, Thierry Tremblay
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
#include "scheduler.hpp"
#include "thread.hpp"


Semaphore::Semaphore(int initialCount)
:   m_count(initialCount),
    m_firstWaiter(nullptr),
    m_lastWaiter(nullptr)
{
    assert(initialCount > 0);
}


void Semaphore::Lock()
{
    m_lock.Lock();

    //Log("Lock(%d)\n", g_scheduler.GetCurrentThread()->id);

    if (m_count > 0)
    {
        // Lock acquired
        --m_count;
        m_lock.Unlock();
    }
    else
    {
        // Blocked - queue current thread and yield
        auto thread = g_scheduler.GetCurrentThread();

        //Log("Lock(%d) - blocking thread\n", thread->id);

        if (m_firstWaiter == nullptr)
        {
            m_lastWaiter = m_firstWaiter = thread;
        }
        else
        {
            m_lastWaiter = m_lastWaiter->next = thread;
        }

        m_lock.Unlock();

        // TODO: it's possible to get pre-empted here, which means inefficient scheduling
        // The problem is we need to unlock the semaphore before we suspend the thread.
        // We need some way to suspend & unlock the semaphore at the same time.
        // Another possibility is to take the scheduler lock around this function, but this
        // isn't going to scale very well...

        g_scheduler.Suspend();

        //Log("Back from suspend(%d)", thread->id);
    }

}


int Semaphore::TryLock()
{
    m_lock.Lock();

    if (m_count > 0)
    {
        // Lock acquired
        --m_count;
        m_lock.Unlock();
        return 1;
    }
    else
    {
        // Failed to lock
        m_lock.Unlock();
        return 0;
    }
}


void Semaphore::Unlock()
{
    m_lock.Lock();

    //Log("Unlock(%d)\n", g_scheduler.GetCurrentThread()->id);

    if (m_firstWaiter == nullptr)
    {
        // No thread waiting, increment counter
        ++m_count;
    }
    else
    {
        // Wake up the oldest blocked thread (first waiter)
        auto thread = m_firstWaiter;
        m_firstWaiter = thread->next;
        if (m_firstWaiter == nullptr)
        {
            m_lastWaiter = nullptr;
        }

        thread->next = nullptr;

        g_scheduler.Wakeup(thread);
    }

    m_lock.Unlock();
}
