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
#include <kernel/kernel.hpp>

extern "C" void interrupt_exit();



bool Task::Initialize(Task* task, EntryPoint entryPoint, const void* args)
{
    /*
        We are going to build multiple frames on the stack
    */

    // TODO: stack guard pages?
    const int stackPageCount = 1;
    const char* stack = (const char*)g_vmm->AllocatePages(stackPageCount);
    if (!stack) return false; // TODO: we should probably do better

    task->kernelStackTop = (uintptr_t)stack;
    task->kernelStackBottom = (uintptr_t)stack + MEMORY_PAGE_SIZE * stackPageCount;

    // TODO: this is not going to work for threads sharing the same address space (we need to allocate stack space from the heap)
    // TODO: use constants, should be 0xF0000000 and not 0xE0000000
    task->userStackTop = 0xE0000000 - 1 * 1024 * 1024; // 1 MB
    task->userStackBottom = 0xE0000000;

    stack = (char*)task->kernelStackBottom;


    /*
        Setup stack for "entryPoint"
    */

    stack -= sizeof(void*);
    *(const void**)stack = args;

    stack -= sizeof(void*);
    *(void**)stack = (void*)Task::Exit;


    /*
        Setup an InterruptContext frame that "returns" to the user's task function.
        This allows us to set all the registers at once.
    */

    // Since we are "returning" to ring 0, ESP and SS won't be popped
    const size_t frameSize = sizeof(InterruptContext) - 2 * sizeof(void*);

    stack = stack - frameSize;

    InterruptContext* frame = (InterruptContext*)stack;

    memset(frame, 0, frameSize);

    frame->cs = GDT_KERNEL_CODE;
    frame->ds = GDT_KERNEL_DATA;
    frame->es = GDT_KERNEL_DATA;    // TODO: probable not needed on x86_64
    frame->fs = GDT_KERNEL_DATA;    // TODO: probable not needed on x86_64
    frame->gs = GDT_KERNEL_DATA;    // TODO: probable not needed on x86_64

    frame->eflags = X86_EFLAGS_IF; // IF = Interrupt Enable
    frame->eip = (uintptr_t)entryPoint;


    /*
        Setup a frame to simulate returning from an interrupt.
    */

    stack -= sizeof(void*);
    *(void**)stack = (void*)interrupt_exit;


    /*
        Setup a TaskRegisters frame to start execution at Task::Entry().
    */

    stack = stack - sizeof(TaskRegisters);
    TaskRegisters* context = (TaskRegisters*)stack;

    context->eip = (uintptr_t)Task::Entry;

    task->context = context;

    return true;
}