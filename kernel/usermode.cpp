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

#include "usermode.hpp"
#include <kernel/kernel.hpp>
#include "elf.hpp"



typedef void (*UserSpaceEntryPoint)();
extern "C" void JumpToUserMode(UserSpaceEntryPoint entryPoint, const void* userArgs, const void* userStack);

extern int SysCallInterrupt(InterruptContext*);



void usermode_init()
{
    interrupt_register(0x80, SysCallInterrupt);
}



static void usermode_entry_spawn(Task* task, void* args)
{
    const auto module = (Module*)args;

    Log("User module at %X, size is %X\n", module->address, module->size);

    const physaddr_t entry = elf_map(module->address, module->size);
    if (!entry)
    {
        Fatal("Could not load / start user process\n");
    }

    Log("Module entry point at %X\n", entry);

// TODO: use constants for these, do not check for arch!
#if defined(__i386__)
    task->userStackTop = 0xE0000000 - 1 * 1024 * 1024; // 1 MB
    task->userStackBottom = 0xE0000000;
#elif defined(__x86_64__)
    task->userStackTop = 0x0000800000000000ull - 1 * 1024 * 1024; // 1 MB
    task->userStackBottom = 0x0000800000000000ull;
#endif

    JumpToUserMode((UserSpaceEntryPoint)entry, nullptr, (void*)task->userStackBottom);
}


void usermode_spawn(const Module* module)
{
    Task::Create(usermode_entry_spawn, module, 0);
}


struct UserCloneContext
{
    const void* entry;
    const void* args;
    int flags;
    const void* userStack;
    size_t userStackSize;
};


static void usermode_entry_clone(Task* task, void* ctx)
{
    const auto context = (UserCloneContext*)ctx;

    const auto entry = context->entry;
    const auto args = context->args;
    //const auto flags = context->flags;
    const auto userStack = context->userStack;
    const auto userStackSize = context->userStackSize;

    Log("User task entry at %p, arg %p, stack at %p\n", entry, args, userStack);

    // TODO: args needs to be passed to the user entry point
    task->userStackTop = (uintptr_t)userStack - userStackSize;
    task->userStackBottom = (uintptr_t)userStack;

    // TODO: this memory allocation is not great...
    free(context);

    JumpToUserMode((UserSpaceEntryPoint)entry, args, userStack);
}


int usermode_clone(const void* userFunction, const void* userArgs, int userFlags, const void* userStack, size_t userStackSize)
{
    // TODO: this memory allocation is not great...
    const auto context = new UserCloneContext();
    context->entry = userFunction;
    context->args = userArgs;
    context->flags = userFlags;
    context->userStack = userStack;
    context->userStackSize = userStackSize;

    if (Task::Create(usermode_entry_clone, context, Task::CREATE_SHARE_PAGE_TABLE))
    {
        return 0;
    }

    return -1;
}
