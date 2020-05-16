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

#ifndef _RAINBOW_KERNEL_SCHEDULER_HPP
#define _RAINBOW_KERNEL_SCHEDULER_HPP

#include <metal/list.hpp>
#include "task.hpp"

class InterruptContext;


/*
    Single CPU scheduler

    Taking the Scheduler Lock means that the current task can't be pre-empted. This
    is accomplished by disabling interrupts.

    Some methods require the caller to first take the scheduler lock. This is to ensure
    that the Scheduler doesn't get pre-empted while manipulating its internal state.
    Methods that require locking will have a note in their comment that says so.

    Other methods do not require the caller to do anything and will do the locking
    internally if required.

    Tasks that are suspended have taken the scheduler lock. This means that interrupts
    are also disabled. When a task becomes active (STATE_RUNNING), it must unlock the
    scheduler. This will re-enable interrupts.
*/

class Scheduler
{
public:

    Scheduler();

    // Initialization
    void Init();

    // Lock the scheduler. This means preventing preemption and protecting scheduling
    // structures, including Task::next.
    void Lock();

    // Unlock the scheduler
    void Unlock();

    // Add a task to this scheduler
    // NOTE: caller is responsible for locking the scheduler before calling this method
    void AddTask(Task* task);

    // Switch execution to the specified task
    // NOTE: caller is responsible for locking the scheduler before calling this method
    void Switch(Task* newTask);

    // Schedule a new task for execution
    // NOTE: caller is responsible for locking the scheduler before calling this method
    void Schedule();

    // Suspend the current task
    void Suspend();

    // Wakeup the specified task (it must be suspended)
    void Wakeup(Task* task);

    // Return whether or not we should call Schedule()
    bool ShouldSchedule() const { return m_switch; }


private:

    static int TimerCallback(InterruptContext* context);

    List<Task>          m_ready;            // List of ready tasks
    int                 m_lockCount;        // Scheduler lock count
    bool                m_enableInterrupts; // Enable interrupts on unlocking?
    bool                m_switch;           // Should we switch task?
};


#endif
