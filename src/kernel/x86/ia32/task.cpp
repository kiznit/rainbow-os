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
#include <kernel/kernel.hpp>
#include <kernel/x86/cpu.hpp>
#include <kernel/x86/selectors.hpp>
#include <rainbow/usertask.h>

extern "C" void interrupt_exit();
extern "C" void task_switch(TaskRegisters** oldContext, TaskRegisters* newContext);


void Task::ArchInit(EntryPoint entryPoint, const void* args)
{
    const char* stack = (char*)GetKernelStack();

    /*
        Setup stack for Task::Entry()
    */

    // Params to Task::Entry()
    stack -= sizeof(void*);
    *(const void**)stack = args;

    stack -= sizeof(void*);
    *(void**)stack = (void*)entryPoint;

    stack -= sizeof(Task*);
    *(const Task**)stack = this;

    // Fake return value - Task::Entry() never returns.
    stack -= sizeof(void*);
    *(void**)stack = nullptr;

    /*
        Setup an interrupt context frame that returns to Task::Entry().
    */

    // Since we are "returning" to ring 0, ESP and SS won't be popped
    const size_t frameSize = sizeof(InterruptContext) - 2 * sizeof(void*);

    stack = stack - frameSize;

    InterruptContext* frame = (InterruptContext*)stack;

    frame->cs = GDT_KERNEL_CODE;
    frame->ds = GDT_KERNEL_DATA;
    frame->es = GDT_KERNEL_DATA;
    frame->fs = GDT_CPU_DATA;
    frame->gs = GDT_TLS;

    frame->eflags = X86_EFLAGS_RESERVED; // Start with interrupts disabled
    frame->eip = (uintptr_t)Task::Entry;

    /*
        Setup a task switch frame to simulate returning from an interrupt.
    */

    stack = stack - sizeof(TaskRegisters);
    TaskRegisters* context = (TaskRegisters*)stack;

    context->eip = (uintptr_t)interrupt_exit;

    m_context = context;
}


void Task::ArchSwitch(Task* currentTask, Task* newTask)
{
    // Stack for interrupts
    Tss32* tss = cpu_get_data(tss);
    tss->esp0 = (uintptr_t)newTask->GetKernelStack();

    // Stack for system calls
    x86_write_msr(MSR_SYSENTER_ESP, (uintptr_t)newTask->GetKernelStack());

    // Page tables
    if (newTask->m_pageTable != currentTask->m_pageTable)
    {
        // TODO: right now this is flushing the entirety of the TLB, not good for performances
        x86_set_cr3(newTask->m_pageTable->m_cr3);
    }

    // TLS
    auto gdt = cpu_get_data(gdt);
    gdt[7].SetUserData32((uintptr_t)newTask->m_userTask, sizeof(UserTask)); // Update GDT entry
    asm volatile ("movl %0, %%gs\n" : : "r" (GDT_TLS) : "memory" );         // Reload GS

    // Switch context
    task_switch(&currentTask->m_context, newTask->m_context);

    assert(!interrupt_enabled());
}
