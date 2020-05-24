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
#include "waitqueue.inl"


Semaphore::Semaphore(int initialCount)
:   m_count(initialCount)
{
    assert(initialCount > 0);
}


void Semaphore::Lock()
{
    //Log("Lock(%d)\n", cpu_get_data(task)->id);

    if (m_count > 0)
    {
        // Lock acquired
        --m_count;
    }
    else
    {
        //Log("Lock(%d) - blocking task\n", task->id);

        g_scheduler->Suspend(m_waiters, Task::STATE_SEMAPHORE);

        //Log("Back from suspend(%d)", task->id);
    }
}


int Semaphore::TryLock()
{
    if (m_count > 0)
    {
        // Lock acquired
        --m_count;
        return 1;
    }
    else
    {
        // Failed to lock
        return 0;
    }
}


void Semaphore::Unlock()
{
    //Log("Unlock(%d)\n", cpu_get_data(task)->id);

    if (m_waiters.empty())
    {
        // No task waiting, increment counter
        ++m_count;
    }
    else
    {
        // Wake up the oldest blocked task (first waiter)
        g_scheduler->Wakeup(m_waiters.front());
    }
}
