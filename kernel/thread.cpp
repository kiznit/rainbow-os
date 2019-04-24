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
#include <kernel/x86/pic.hpp>
#include <metal/arch.hpp>
#include <metal/crt.hpp>
#include <metal/log.hpp>
#include "scheduler.hpp"
#include "mutex.hpp"
#include "timer.hpp"


#if defined(__i386__)
#include "x86/ia32/interrupt.hpp"
#elif defined(__x86_64__)
#include "x86/x86_64/interrupt.hpp"
#endif


extern "C" void interrupt_exit();


static Thread g_thread1;
static Thread g_thread2;
static char g_stack1[65536];
static char g_stack2[65536];


static int timer_callback(InterruptController* controller, InterruptContext* context)
{
    Log("*");
    (void)context;

    const Thread* thread = g_scheduler.GetCurrentThread();

    g_scheduler.Lock();

    (void)controller;
//    controller->Enable(context->interrupt - PIC_IRQ_OFFSET); // TODO: shouldn't know about PIC offset

    // What we want here is ensure we don't schedule a new thread
    // if a thread switch occured while we were waiting for the
    // scheduler lock.
// TODO: make this check foolproof
    if (g_scheduler.GetCurrentThread() == thread)
    {
        g_scheduler.Schedule();
    }

    g_scheduler.Unlock();

    return 1;
}


static Mutex g_mutex;


static void ThreadFunction0()
{
    for (;;)
    {
        g_mutex.Lock();
        Log("0");
        g_mutex.Unlock();
    }
}


static void ThreadFunction1()
{
    for (;;)
    {
        g_mutex.Lock();
        Log("1");
        g_mutex.Unlock();
    }
}


static void ThreadFunction2()
{
    for (;;)
    {
        g_mutex.Lock();
        Log("2");
        g_mutex.Unlock();
    }
}


void thread_init()
{
    g_timer->Initialize(1, timer_callback);

    thread_create(ThreadFunction1);
    thread_create(ThreadFunction2);

    ThreadFunction0();
}


// Entry point for all threads.
static void thread_entry()
{
    Log("thread_entry(%d)\n", g_scheduler.GetCurrentThread()->id);

    // We got here immediately after a call to Scheduler::Switch().
    // This means we still have the scheduler lock and we must release it.
    g_scheduler.Unlock();
}


// Exit point for threads that exit normally (returning from their thread function).
static void thread_exit()
{
    Log("thread_exit(%d)\n", g_scheduler.GetCurrentThread()->id);

    //todo: kill current thread (i.e. zombify it)
    //todo: remove thread from scheduler
    //todo: yield() / schedule()

    //todo
    //cpu_halt();
    for (;;);
}


static volatile unsigned g_nextThreadId = 0;


Thread* thread_create(ThreadFunction userThreadFunction)
{
    //TODO
    //Thread* thread = ... allocate new thread object
    Thread* thread = userThreadFunction == ThreadFunction1 ? &g_thread1 : &g_thread2;

    /*
        We are going to build multiple frames on the stack
    */

    //todo: proper stack allocation with guard pages
    const char* stack = userThreadFunction == ThreadFunction1 ? g_stack1 + sizeof(g_stack1) : g_stack2 + sizeof(g_stack2);


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
    frame->eip = (uintptr_t)userThreadFunction;
    frame->esp = (uintptr_t)(stack + sizeof(InterruptContext));
#elif defined(__x86_64__)
    frame->rflags = X86_EFLAGS_IF; // IF = Interrupt Enable
    frame->rip = (uintptr_t)userThreadFunction;
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
    thread->id = __sync_add_and_fetch(&g_nextThreadId, 1);
    thread->state = THREAD_READY;
    thread->context = context;
    thread->next = nullptr;


    /*
        Queue this new thread
    */

    g_scheduler.Lock();
    g_scheduler.AddThread(thread);
    g_scheduler.Unlock();

    return thread;
}
