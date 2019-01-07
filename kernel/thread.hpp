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

#ifndef _RAINBOW_KERNEL_THREAD_HPP
#define _RAINBOW_KERNEL_THREAD_HPP

#if defined(__i386__)
#include "x86/ia32/thread.hpp"
#elif defined(__x86_64__)
#include "x86/x86_64/thread.hpp"
#endif


typedef void (*ThreadFunction)();


enum ThreadState
{
    THREAD_RUNNING,
    THREAD_READY,
    THREAD_SUSPENDED,
};


struct Thread
{
    ThreadState         state;      // Scheduling state
    ThreadRegisters*    context;    // Saved context (on the thread's stack)
};


// Initialize scheduler
void thread_init();

// Create a new thread
Thread* thread_create(ThreadFunction userThreadFunction);

// Retrieve the currently running thread
Thread* thread_current();

// Yield the CPU to another thread
void thread_yield();


#endif
