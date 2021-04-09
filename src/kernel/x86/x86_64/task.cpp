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

#include "task.hpp"

#include <cassert>
#include <cstring>
#include <metal/helpers.hpp>

#include "kernel.hpp"
#include "x86/cpu.hpp"
#include "x86/selectors.hpp"

extern "C" void interrupt_exit();
extern "C" void task_switch(TaskRegisters** oldContext, TaskRegisters* newContext);


void Task::ArchInit(EntryPoint entryPoint, const void* args)
{
    const char* stack = (char*)GetKernelStack();

    /*
        Setup an interrupt context frame that returns to Task::Entry().
    */

    const size_t frameSize = sizeof(InterruptContext);

    stack = stack - align_up(frameSize, 16);

    InterruptContext* frame = (InterruptContext*)stack;

    frame->cs = GDT_KERNEL_CODE;
    frame->rflags = X86_EFLAGS_RESERVED; // Start with interrupts disabled
    frame->rip = (uintptr_t)Task::Entry;

    // Params to Task::Entry()
    frame->rdi = (uintptr_t)this;
    frame->rsi = (uintptr_t)entryPoint;
    frame->rdx = (uintptr_t)args;

    // In long mode, rsp and ss are always popped on iretq
    frame->rsp = (uintptr_t)(stack + frameSize);
    frame->ss = GDT_KERNEL_DATA;


    /*
        Setup a task switch frame to simulate returning from an interrupt.
    */

    stack = stack - sizeof(TaskRegisters);
    TaskRegisters* context = (TaskRegisters*)stack;

    context->rip = (uintptr_t)interrupt_exit;

    m_context = context;
}


void Task::ArchSwitch(Task* currentTask, Task* newTask)
{
    // Stack for interrupts
    Tss64* tss = cpu_get_data(tss);
    tss->rsp0 = (uintptr_t)newTask->GetKernelStack();

    // Stack for system calls
    cpu_set_data(kernelStack, newTask->GetKernelStack());

    // Page tables
    if (newTask->m_pageTable != currentTask->m_pageTable)
    {
        // TODO: right now this is flushing the entirety of the TLB, not good for performances
        x86_set_cr3(newTask->m_pageTable->m_cr3);
    }

    // TLS
    x86_write_msr(Msr::IA32_FS_BASE, (uintptr_t)newTask->m_userTask);

    // Switch context
    task_switch(&currentTask->m_context, newTask->m_context);

    assert(!interrupt_enabled());
}
