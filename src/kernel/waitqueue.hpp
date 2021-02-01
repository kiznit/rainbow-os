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

#ifndef _RAINBOW_KERNEL_WAITQUEUE_HPP
#define _RAINBOW_KERNEL_WAITQUEUE_HPP

#include <cstdint>
#include <memory>
#include <vector>
#include <kernel/spinlock.hpp>
#include <kernel/taskdefs.hpp>

class Task;


// For now this is just a thin wrapper around std::vector<Task*>.
// Eventually we will add synchronization primitives and more logic to this class.

class WaitQueue
{
public:

    WaitQueue() {}
    ~WaitQueue();

    // Non-copyable
    WaitQueue(const WaitQueue&) = delete;
    WaitQueue& operator=(const WaitQueue&) = delete;

    // Suspend the current task.
    // The task will be queued and it's state updated.
    // Use 'nextTask' to give a hint about which task should run next.
    void Suspend(TaskState reason/*, Task* nextTask = nullptr*/);

    // Wake up the specified task (it must be suspended and in this queue!)
    // The task will be removed from this queue and put back into a run queue.
    void Wakeup(Task* task);

    // Wake up one task (if there is any available)
    void WakeupOne();

    // Wake up all tasks
    void WakeupAll();

    // Wake up tasks whose sleep time is expired
    // TODO: we want to make the timeout functionality generic
    void WakeupUntil(uint64_t timeNs);

    // Remove the last entry
    // TODO: this is only used for killing zombies, can we do this in a better way?
    std::unique_ptr<Task> PopBack();

// TODO: eliminate old interface
    Task* front() const { return m_tasks.empty() ? nullptr : m_tasks.front().get(); }


private:

    Spinlock                           m_lock;
    std::vector<std::unique_ptr<Task>> m_tasks;
};


#endif
