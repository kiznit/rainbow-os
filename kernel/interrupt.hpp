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

#ifndef _RAINBOW_KERNEL_INTERRUPT_HPP
#define _RAINBOW_KERNEL_INTERRUPT_HPP

#if defined(__i386__)
#include "x86/ia32/interrupt.hpp"
#elif defined(__x86_64__)
#include "x86/x86_64/interrupt.hpp"
#endif



struct InterruptController
{
    // The 'baseInterruptOffset' is the base offset into the interrupt descriptor table (IDT)
    virtual void Initialize(int baseInterruptOffset) = 0;

    // Is the interrupt spurious?
    // TODO: how can I hide this inside the implementation and remove from this interface?
    virtual bool IsSpurious(int interrupt) = 0;

    // Acknowledge an interrupt (End of interrupt / EOI)
    virtual void Acknowledge(int interrupt) = 0;

    // Enable the specified interrupt
    virtual void Enable(int interrupt) = 0;

    // Disable the specified interrupt
    virtual void Disable(int interrupt) = 0;
};


extern InterruptController* g_interruptController;


// An interrupt handler should return 0 for "not handled" and 1 for "handled".
typedef int (*InterruptHandler)(InterruptContext*);


// Initialize interrupt vectors
void interrupt_init();


// Register an interrupt service routine.
// Returns 0 on error (there is already an interrupt handler for the specified interrupt)
int interrupt_register(int interrupt, InterruptHandler handler);


#endif
