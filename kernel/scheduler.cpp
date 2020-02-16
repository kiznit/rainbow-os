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
#include <kernel/kernel.hpp>
#include <kernel/x86/pic.hpp>

extern "C" void thread_switch(ThreadRegisters** oldContext, ThreadRegisters* newContext);

extern "C" void JumpToUserMode(void (*entryPoint)(), void* userStack);
extern Tss g_tss;



static int TimerCallback(InterruptController* controller, InterruptContext* context)
{
    (void)context;

    g_scheduler->Lock();

    controller->Enable(context->interrupt - PIC_IRQ_OFFSET); // TODO: shouldn't know about PIC offset

    // TODO: here we would like to detect whether or not thread
    // switches happened while we were waiting for the scheduler
    // lock. If that is the case, we do not want to call Schedule().
    g_scheduler->Schedule();

    g_scheduler->Unlock();

    return 1;
}


Scheduler::Scheduler()
:   m_current(Thread::InitThread0()),
    m_lockCount(0)
{
}


void Scheduler::Init()
{
    g_timer->Initialize(1, TimerCallback);
}


void Scheduler::Lock()
{
    interrupt_disable();
    ++m_lockCount;
}


void Scheduler::Unlock()
{
    if (--m_lockCount == 0)
    {
        interrupt_enable();
    }
}


void Scheduler::AddThread(Thread* thread)
{
    assert(m_lockCount > 0);
    assert(thread->next == nullptr);

    if (thread->state == Thread::STATE_RUNNING)
    {
        m_current = thread;
    }
    else
    {
        assert(thread->state == Thread::STATE_READY);
        m_ready.push_back(thread);
    }
}


void Scheduler::Switch(Thread* newThread)
{
    //Log("Switch(%d), state %d\n", newThread->id, newThread->state);

    assert(m_lockCount > 0);
    assert(!interrupt_enabled());
    assert(newThread->state == Thread::STATE_READY);

    if (m_current == newThread)
    {
        // If the current thread isn't running, we might have a problem?
        assert(m_current->state == Thread::STATE_RUNNING);
        return;
    }

    // TODO: right now we only have a "ready" list, but eventually we will need to remove the thread from the right list
    m_ready.remove(newThread);

    auto oldThread = m_current;
    if (oldThread->state == Thread::STATE_RUNNING)
    {
        oldThread->state = Thread::STATE_READY;
        m_ready.push_back(oldThread);
    }
    else
    {
        assert(oldThread->state == Thread::STATE_SUSPENDED);
    }

    newThread->state = Thread::STATE_RUNNING;
    m_current = newThread;

    newThread->pageTable.Enable();

    thread_switch(&oldThread->context, newThread->context);
}


void Scheduler::Schedule()
{
    assert(m_current->state == Thread::STATE_RUNNING || m_current->state == Thread::STATE_SUSPENDED);
    assert(m_current->next == nullptr);

    assert(m_lockCount > 0);
    assert(!interrupt_enabled());

    if (m_ready.empty())
    {
        return;
    }

    Switch(m_ready.front());
}


void Scheduler::Suspend()
{
    Lock();

    //Log("Suspend(%d)\n", m_current->id);

    assert(m_current->state == Thread::STATE_RUNNING);
    assert(m_current->next == nullptr);

    m_current->state = Thread::STATE_SUSPENDED;
    Schedule();

    Unlock();
}


void Scheduler::Wakeup(Thread* thread)
{
    Lock();

    //Log("Wakeup(%d), state %d\n", thread->id, thread->state);

    assert(thread->state == Thread::STATE_SUSPENDED);
    assert(thread->next == nullptr);

    // TODO: maybe we want to prempt the current thread and execute the unblocked one
    thread->state = Thread::STATE_READY;
    m_ready.push_back(thread);

    Unlock();
}
