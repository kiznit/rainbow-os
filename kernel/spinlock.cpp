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

#include "spinlock.hpp"
#include <cassert>
#include <climits>
#include <mutex>
#include <kernel/task.hpp>
#include <metal/arch.hpp>


void Spinlock::lock()
{
    // We can't have interrupts enabled as being preempted would cause deadlocks.
    assert(!interrupt_enabled());

    // TODO: ensure the task with the lock doesn't yield / is not preempted using asserts

    while (!try_lock())
    {
        // TODO: this is x86 specific, replace with generic helper (pause() or usleep() or ...)
        // TODO: do we need this? It was added when we were using the LOCK prefix, but we aren't anymore...
        x86_pause();
    }
}


bool Spinlock::try_lock()
{
    // We can't have interrupts enabled as being preempted would cause deadlocks.
    assert(!interrupt_enabled());

    return !m_lock.exchange(true, std::memory_order_acquire);
}


void Spinlock::unlock()
{
    // We can't have interrupts enabled as being preempted would cause deadlocks.
    assert(!interrupt_enabled());

    assert(m_lock); // TODO: can we verify that we own the lock?

    m_lock.store(false, std::memory_order_release);
}


RecursiveSpinlockImpl::RecursiveSpinlockImpl()
:   m_owner(-1),
    m_count(0)
{
}


void RecursiveSpinlockImpl::lock(int owner)
{
    // TODO: ensure the task with the lock doesn't yield / is not preempted using asserts

    while (!try_lock(owner))
    {
        // TODO: this is x86 specific, replace with generic helper (pause() or usleep() or ...)
        // TODO: do we need this? It was added when we were using the LOCK prefix, but we aren't anymore...
        x86_pause();
    }
}


bool RecursiveSpinlockImpl::try_lock(int owner)
{
    std::lock_guard lock(m_lock);

    if (m_owner == -1)
    {
        m_owner = owner;
        m_count = 1;
        return true;
    }
    else if (m_owner == owner && m_count < INT_MAX)
    {
        ++m_count;
        return true;
    }
    else
    {
        return false;
    }
}


void RecursiveSpinlockImpl::unlock(int owner)
{
    std::lock_guard lock(m_lock);

    assert(m_owner == owner);
    assert(m_count > 0);

    if (--m_count == 0)
    {
        m_owner = -1;
    }
}
