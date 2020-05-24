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


class Scheduler
{
public:

    Scheduler();

    // Initialization
    void Init();

    // Add a task to this scheduler
    void AddTask(Task* task);

    // Switch execution to the specified task
    void Switch(Task* newTask);

    // Schedule a new task for execution
    void Schedule();

    // Return whether or not we should call Schedule()
    bool ShouldSchedule() const { return m_switch; }

    // Suspend the current task.
    // The task will be put in the specified queue and its state updated.
    // Use 'nextTask' to give a hint about which task should run next.
    // NOTE: make sure the current task was stored in a wait list (i.e. Waitable)
    void Suspend(WaitQueue& queue, Task::State reason, Task* nextTask = nullptr);

    // Wakeup the specified task (it must be suspended!)
    // The task will be removed from its waiting queue and put back into the ready queue.
    void Wakeup(Task* task);

    // Yield the CPU to another thread
    void Yield();


private:

    static int TimerCallback(InterruptContext* context);

    WaitQueue           m_ready;                // List of ready tasks - TODO: should this be a "WaitQueue"?
    bool                m_switch;               // Should we switch task?

// TODO: ugly!!!
public:
    WaitQueue           m_ipcWaiters;           // List of tasks blocked on ipc_wait
};


#endif
