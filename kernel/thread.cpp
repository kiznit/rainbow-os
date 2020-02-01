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
#include <metal/arch.hpp>
#include <metal/crt.hpp>
#include <metal/log.hpp>
#include "kernel.hpp"

extern "C" void interrupt_exit();


// Entry point for all threads.
static void thread_entry()
{
    Log("thread_entry(%d)\n", g_scheduler->GetCurrentThread()->id);

    // We got here immediately after a call to Scheduler::Switch().
    // This means we still have the scheduler lock and we must release it.
    g_scheduler->Unlock();
}


// Exit point for threads that exit normally (returning from their thread function).
static void thread_exit()
{
    Log("thread_exit(%d)\n", g_scheduler->GetCurrentThread()->id);

    //todo: kill current thread (i.e. zombify it)
    //todo: remove thread from scheduler
    //todo: yield() / schedule()
    //todo: free the kernel stack
    //todo: free the thread

    //todo
    //cpu_halt();
    for (;;);
}


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



Thread* Thread::Create(EntryPoint entryPoint)
{
    Thread* thread = new Thread();
    if (!thread)
    {
        return nullptr;
    }

    /*
        We are going to build multiple frames on the stack
    */
    const auto stackSize = sizeof(void*) * 1024;
    const char* stack = (const char*)g_vmm->m_kernelMemoryMap->AllocateStack(stackSize);

    if (!stack)
    {
        delete thread;
        return nullptr;
    }

    thread->kernelStackTop = stack - stackSize;
    thread->kernelStackBottom = stack;


    /*
        Setup the last frame to execute thread_exit().
    */

    stack -= sizeof(void*);
    *(void**)stack = (void*)thread_exit;


    /*
        Setup an InterruptContext frame that "returns" to the user's thread function.
        This allows us to set all the registers at once.
    */

    stack = stack - sizeof(InterruptContext);

    InterruptContext* frame = (InterruptContext*)stack;

    memset(frame, 0, sizeof(*frame));

    frame->cs = GDT_KERNEL_CODE;
    frame->ss = GDT_KERNEL_DATA;
    frame->ds = GDT_KERNEL_DATA;
    frame->es = GDT_KERNEL_DATA;
    frame->fs = GDT_KERNEL_DATA;
    frame->gs = GDT_KERNEL_DATA;

#if defined(__i386__)
    frame->eflags = X86_EFLAGS_IF; // IF = Interrupt Enable
    frame->eip = (uintptr_t)entryPoint;
    frame->esp = (uintptr_t)(stack + sizeof(InterruptContext));
#elif defined(__x86_64__)
    frame->rflags = X86_EFLAGS_IF; // IF = Interrupt Enable
    frame->rip = (uintptr_t)entryPoint;
    frame->rsp = (uintptr_t)(stack + sizeof(InterruptContext));
#endif


    /*
        Setup a frame so that thread_entry() simulates returning from an interrupt.
    */

    stack -= sizeof(void*);
    *(void**)stack = (void*)interrupt_exit;


    /*
        Setup a ThreadRegisters frame to start execution at thread_entry().
    */

    stack = stack - sizeof(ThreadRegisters);
    ThreadRegisters* context = (ThreadRegisters*)stack;

#if defined(__i386__)
    context->eip = (uintptr_t)thread_entry;
#elif defined(__x86_64__)
    context->rip = (uintptr_t)thread_entry;
#endif


    // Initialize thread object
    thread->id = __sync_add_and_fetch(&s_nextThreadId, 1);
    thread->state = STATE_READY;
    thread->context = context;
    thread->next = nullptr;

    assert(thread->id < ARRAY_LENGTH(s_threads));
    s_threads[thread->id] = thread;


    /*
        Queue this new thread
    */

    g_scheduler->Lock();
    g_scheduler->AddThread(thread);
    g_scheduler->Unlock();

    return thread;
}
