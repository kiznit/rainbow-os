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
#include <metal/arch.hpp>


void Spinlock::Lock()
{
    // We can't have interrupts enabled as being preempted would cause deadlocks.
    assert(!interrupt_enabled());

// TODO: ensure the task with the lock doesn't yield / is not preempted using asserts

    while (m_lock.exchange(true, std::memory_order_acquire))
    {
        // TODO: this is x86 specific, replace with generic helper (pause() or usleep() or ...)
        // TODO: do we need this? It was added when we were using the LOCK prefix, but we aren't anymore...
        x86_pause();
    }
}


bool Spinlock::TryLock()
{
    // We can't have interrupts enabled as being preempted would cause deadlocks.
    assert(!interrupt_enabled());

    return !m_lock.exchange(true, std::memory_order_acquire);
}


void Spinlock::Unlock()
{
    // We can't have interrupts enabled as being preempted would cause deadlocks.
    assert(!interrupt_enabled());

    assert(m_lock); // TODO: can we verify that we own the lock?

    m_lock.store(false, std::memory_order_release);
}
