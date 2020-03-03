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

#ifndef _RAINBOW_KERNEL_SCHEDULER_HPP
#define _RAINBOW_KERNEL_SCHEDULER_HPP

#include <metal/list.hpp>
#include "thread.hpp"

class InterruptContext;


/*
    Single CPU scheduler

    Taking the Scheduler Lock means that the current thread can't be pre-empted. This
    is accomplished by disabling interrupts.

    Some methods require the caller to first take the scheduler lock. This is to ensure
    that the Scheduler doesn't get pre-empted while manipulating its internal state.
    Methods that require locking will have a note in their comment that says so.

    Other methods do not require the caller to do anything and will do the locking
    internally if required.

    Threads that are suspended have taken the scheduler lock. This means that interrupts
    are also disabled. When a thread becomes active (THREAD_RUNNING), it must unlock the
    scheduler. This will re-enable interrupts.
*/

class Scheduler
{
public:

    Scheduler();

    // Initialization
    void Init();

    // Lock the scheduler. This means preventing preemption and protecting scheduling
    // structures, including Thread::next.
    void Lock();

    // Unlock the scheduler
    void Unlock();

    // Add a thread to this scheduler
    // NOTE: caller is responsible for locking the scheduler before calling this method
    void AddThread(Thread* thread);

    // Switch execution to the specified thread
    // NOTE: caller is responsible for locking the scheduler before calling this method
    void Switch(Thread* newThread);

    // Schedule a new thread for execution
    // NOTE: caller is responsible for locking the scheduler before calling this method
    void Schedule();

    // Suspend the current thread
    void Suspend();

    // Wakeup the specified thread (it must be suspended)
    void Wakeup(Thread* thread);

    // Return the currently running thread
    Thread* GetCurrentThread() const { return m_current; }

    // Return whether or not we should call Schedule()
    bool ShouldSchedule() const { return m_switch; }


private:

    static int TimerCallback(InterruptContext* context);

    Thread* volatile    m_current;      // Current running thread
    List<Thread>        m_ready;        // List of ready threads
    int                 m_lockCount;    // Scheduler lock count
    int                 m_switch;       // Should we switch thread?
};


#endif
