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
#include "cpu.hpp"
#include <kernel/biglock.hpp>
#include <kernel/kernel.hpp>
#include <kernel/x86/selectors.hpp>

extern "C" void interrupt_exit();
extern "C" void task_switch(TaskRegisters** oldContext, TaskRegisters* newContext);



bool Task::Initialize(Task* task, EntryPoint entryPoint, const void* args)
{
    const char* stack = (char*)task->GetKernelStack();

    /*
        Setup stack for Task::Entry()
    */

    // Params to Task::Entry()
    stack -= sizeof(void*);
    *(const void**)stack = args;

    stack -= sizeof(void*);
    *(void**)stack = (void*)entryPoint;

    stack -= sizeof(Task*);
    *(const Task**)stack = task;

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

    memset(frame, 0, frameSize);

    frame->cs = GDT_KERNEL_CODE;
    frame->ds = GDT_KERNEL_DATA;
    frame->es = GDT_KERNEL_DATA;
    frame->fs = GDT_KERNEL_DATA;
    frame->gs = GDT_PER_CPU;

    frame->eflags = X86_EFLAGS_RESERVED; // Start with interrupts disabled
    frame->eip = (uintptr_t)Task::Entry;

    /*
        Setup a task switch frame to simulate returning from an interrupt.
    */

    stack = stack - sizeof(TaskRegisters);
    TaskRegisters* context = (TaskRegisters*)stack;

    context->eip = (uintptr_t)interrupt_exit;

    task->context = context;

    return true;
}


void Task::Switch(Task* currentTask, Task* newTask)
{
    // Save FPU state
    // TODO: investigate better method: XSAVES > XSAVEOPT > XSAVEC > XSAVE > FXSAVE
    x86_fxsave(&currentTask->fpuState);

    // Stack for interrupts
    Tss32* tss = cpu_get_data(tss);
    tss->esp0 = (uintptr_t)newTask->GetKernelStack();

    // Stack for system calls
    x86_write_msr(MSR_SYSENTER_ESP, (uintptr_t)newTask->GetKernelStack());

    // Page tables
    if (newTask->pageTable.cr3 != currentTask->pageTable.cr3)
    {
        // TODO: right now this is flushing the entirety of the TLB, not good for performances
        assert(newTask->pageTable.cr3);
        x86_set_cr3(newTask->pageTable.cr3);
    }

    assert(g_bigKernelLock.IsLocked());
    g_bigKernelLock.Unlock();

    // Switch context
    task_switch(&currentTask->context, newTask->context);

    assert(!interrupt_enabled());
    g_bigKernelLock.Lock();

    // Restore FPU state
    x86_fxrstor(&currentTask->fpuState);
}
