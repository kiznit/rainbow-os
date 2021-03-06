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
#include <metal/interrupt.hpp>

#include "kernel.hpp"
#include "task.hpp"


void Spinlock::lock()
{
    // TODO: ensure the task with the lock doesn't yield / is not preempted using asserts

    while (m_lock.load() || !try_lock())
    {
        // TODO: do we need this? It was added when we were using the LOCK prefix, but we aren't anymore...
        cpu_pause();
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

    assert(m_lock);

    m_lock.store(false, std::memory_order_release);
}


RecursiveSpinlock::RecursiveSpinlock()
:   m_owner(-1),
    m_count(0)
{
}


void RecursiveSpinlock::lock()
{
    // TODO: ensure the task with the lock doesn't yield / is not preempted using asserts

    while (!try_lock())
    {
        // TODO: do we need this? It was added when we were using the LOCK prefix, but we aren't anymore...
        cpu_pause();
    }
}


bool RecursiveSpinlock::try_lock()
{
    // We can't have interrupts enabled as being preempted would cause deadlocks.
    assert(!interrupt_enabled());

    const auto cpuId = cpu_get_data(id);

    if (m_owner == cpuId)
    {
        ++m_count;
        return true;
    }

    if (!m_lock.exchange(true, std::memory_order_acquire))
    {
        assert(m_count == 0);

        m_owner = cpuId;
        return true;
    }

    return false;
}


void RecursiveSpinlock::unlock()
{
    // We can't have interrupts enabled as being preempted would cause deadlocks.
    assert(!interrupt_enabled());

    const auto cpuId = cpu_get_data(id);

    assert(m_owner == cpuId);

    if (m_count)
    {
        --m_count;
        return;
    }

    assert(m_count == 0);

    m_owner = -1;
    m_lock.store(false, std::memory_order_release);
}
