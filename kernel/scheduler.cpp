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


#include "scheduler.hpp"

extern "C" void thread_switch(ThreadRegisters** oldContext, ThreadRegisters* newContext);


static Thread g_thread0;    // Initial thread


Scheduler::Scheduler()
{
    // Setup the initial thread
    g_thread0.state = THREAD_RUNNING;
    g_thread0.context = nullptr;
    g_thread0.next = nullptr;

    m_current = &g_thread0;
}


void Scheduler::AddThread(Thread* thread)
{
    if (thread->state == THREAD_RUNNING)
    {
        m_current = thread;
    }
    else
    {
//TODO: assert(thread->state == THREAD_READY);
        m_ready.push_back(thread);
    }
}


void Scheduler::Schedule()
{
    // TODO: bunch of checks, see kiznix

    m_ready.push_back(m_current);

    auto newThread = m_ready.pop_front();
    auto oldThread = m_current;

    // Note: it is possible for newThread == oldThread, so careful with ordering here!
    oldThread->state = THREAD_READY;
    newThread->state = THREAD_RUNNING;

    m_current = newThread;

    thread_switch(&oldThread->context, newThread->context);
}
