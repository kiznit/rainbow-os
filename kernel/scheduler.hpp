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



class Scheduler
{
public:

    Scheduler();

    // Add a thread to this scheduler
    void AddThread(Thread* thread);

    Thread* GetCurrentThread() const { return m_current; }

    // Schedule a new thread for execution
    // NOTE: caller is responsible for locking the scheduler before calling this method
    void Schedule();

    // Switch execution to the specified thread
    // NOTE: caller is responsible for locking the scheduler before calling this method
    void Switch(Thread* newThread);

    // Lock / unlock the scheduler
    void Lock();
    void Unlock();


private:

    Thread* volatile    m_current;      // Current running thread
    List<Thread>        m_ready;        // List of ready threads
    int                 m_lockCount;    // Scheduler lock count
};


#endif
