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
#include "timer.hpp"


#if defined(__i386__)
#include "x86/ia32/interrupt.hpp"
#elif defined(__x86_64__)
#include "x86/x86_64/interrupt.hpp"
#endif


extern "C" void interrupt_exit();


static Thread g_thread1;
static char g_stack[65536];


static Scheduler g_scheduler;


static int timer_callback(InterruptContext* context)
{
    (void)context;

    g_scheduler.Lock();
    pic_enable_irq(0);
    g_scheduler.Schedule();
    g_scheduler.Unlock();

    return 1;
}


static void ThreadFunction0()
{
    for (;;)
    {
        Log("0");
    }
}


static void ThreadFunction1()
{
    for (;;)
    {
        Log("1");
    }
}



void thread_init()
{
    timer_init(1, timer_callback);

    thread_create(ThreadFunction1);

    ThreadFunction0();
}


// Entry point for all threads.
static void thread_entry()
{
    //Log("%p: thread_entry()\n", thread_current());

    // We got here immediately after a call to thread_switch().
    // This means we still have the scheduler lock and we must release it.
//    spin_unlock(&scheduler_lock);

// TODO: This is wrong, the issue is that interrupt_dispatch() will disable the PIC
    // IRQ 0 (PIT) is disabled at this point, re-enable it
    //pic_enable_irq(0);
}



// Exit point for threads that exit normally (returning from their thread function).
static void thread_exit()
{
    //Log("%p: thread_exit()\n", thread_current());

    //todo: kill current thread (i.e. zombify it)
    // todo: remove thread from scheduler
    //todo: yield() / schedule()

    //todo
    //cpu_halt();
    for (;;);
}



Thread* thread_create(ThreadFunction userThreadFunction)
{
    //TODO
    //Thread* thread = ... allocate new thread object
    Thread* thread = &g_thread1;

    thread->state = THREAD_READY;

    /*
        We are going to build multiple frames on the stack
    */

    //todo: proper stack allocation with guard pages
    const char* stack = g_stack + sizeof(g_stack);


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

    frame->cs = 0x08;   //todo: no hard coded constant! use GDT_KERNEL_CODE
    frame->ss = 0x10;   //todo: no hard coded constant! use GDT_KERNEL_DATA
    frame->ds = 0x10;   //todo: no hard coded constant! use GDT_KERNEL_DATA
    frame->es = 0x10;   //todo: no hard coded constant! use GDT_KERNEL_DATA
    frame->fs = 0x10;   //todo: no hard coded constant! use GDT_KERNEL_DATA
    frame->gs = 0x10;   //todo: no hard coded constant! use GDT_KERNEL_DATA

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

    thread->context = context;
    thread->next = nullptr;


    /*
        Queue this new thread
    */

    g_scheduler.AddThread(thread);

    return thread;
}
