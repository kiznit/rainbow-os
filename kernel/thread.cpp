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

#include "thread.hpp"
#include <kernel/kernel.hpp>


static volatile Thread::Id s_nextThreadId = 0;


static Thread s_thread0;        // Initial kernel thread

// TODO: this is temporary until we have a proper associative structure (hashmap?)
static Thread* s_threads[100];



Thread* Thread::Get(Id id)
{
    if (id < ARRAY_LENGTH(s_threads))
        return s_threads[id];
    else
        return nullptr;
}



Thread* Thread::InitThread0()
{
    Thread* thread = &s_thread0;

    thread->id = 0;
    thread->state = STATE_RUNNING;
    thread->context = nullptr;

    // TODO
    thread->kernelStackTop = nullptr;
    thread->kernelStackBottom = nullptr;

    thread->next = nullptr;

    s_threads[0] = thread;

    return thread;
}



Thread* Thread::Create(EntryPoint entryPoint, void* entryContext)
{
    // Allocate
    auto thread = new Thread();
    if (!thread) return nullptr; // TODO: we should probably do better

    // Initialize
    memset(thread, 0, sizeof(*thread));
    thread->id = __sync_add_and_fetch(&s_nextThreadId, 1);
    thread->state = STATE_INIT;

    assert(thread->id < ARRAY_LENGTH(s_threads));
    s_threads[thread->id] = thread;

    if (!Bootstrap(thread, entryPoint, entryContext))
    {
        // TODO: we should probably do better
        delete thread;
        return nullptr;
    }

    // Schedule the thread
    g_scheduler->Lock();
    thread->state = STATE_READY;
    g_scheduler->AddThread(thread);
    g_scheduler->Unlock();

    return thread;
}



void Thread::Entry()
{
    Log("Thread::Entry(%d)\n", g_scheduler->GetCurrentThread()->id);

    // We got here immediately after a call to Scheduler::Switch().
    // This means we still have the scheduler lock and we must release it.
    g_scheduler->Unlock();
}



void Thread::Exit()
{
    Log("Thread::Entry(%d)\n", g_scheduler->GetCurrentThread()->id);

    //todo: kill current thread (i.e. zombify it)
    //todo: remove thread from scheduler
    //todo: yield() / schedule()
    //todo: free the kernel stack
    //todo: free the thread

    //todo
    //cpu_halt();
    for (;;);
}
