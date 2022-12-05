/*
    Copyright (c) 2022, Thierry Tremblay
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
#include "Cpu.hpp"
#include "Task.hpp"
#include "interrupt.hpp"
#include <metal/helpers.hpp>

extern "C" void InterruptExit();
extern "C" void TaskSwitch(TaskContext** oldContext, TaskContext* newContext);

void Task::Initialize(EntryPoint entryPoint, const void* args)
{
    const char* stack = (char*)GetStack();

    // We use an InterruptContext interruptContext to "return" to the task's entry point. The reason we can't only use a TaskContext
    // interruptContext is that we need to be able to set arguments for the entry point. These need to go in registers (rdi, rsi,
    // rdx) that aren't part of the TaskContext.
    constexpr auto interruptContextSize = sizeof(InterruptContext);
    stack = stack - mtl::AlignUp(interruptContextSize, 16);

    const auto interruptContext = (InterruptContext*)stack;
    interruptContext->rip = (uintptr_t)Task::Entry;                    // "Return" to Task::Entry
    interruptContext->cs = (uint64_t)Selector::KernelCode;             // "Return" to kernel code
    interruptContext->rflags = mtl::EFLAGS_RESERVED;                   // Start with interrupts disabled
    interruptContext->rsp = (uintptr_t)(stack + interruptContextSize); // Required by iretq
    interruptContext->ss = (uint64_t)Selector::KernelData;             // Required by iretq
    interruptContext->rdi = (uintptr_t)this;                           // Param 1 for Task::Entry
    interruptContext->rsi = (uintptr_t)entryPoint;                     // Param 2 for Task::Entry
    interruptContext->rdx = (uintptr_t)args;                           // Param 3 for Task::Entry

    // Setup a task switch interruptContext to simulate returning from an interrupt.
    stack = stack - sizeof(TaskContext);

    const auto taskContext = (TaskContext*)stack;
    taskContext->rip = (uintptr_t)InterruptExit;

    m_context = taskContext;
}

void Task::SwitchTo(Task* newTask)
{
    TaskSwitch(&m_context, newTask->m_context);
}
