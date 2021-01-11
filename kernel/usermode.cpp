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
#include <kernel/biglock.hpp>
#include <kernel/config.hpp>
#include <kernel/kernel.hpp>
#include <kernel/vdso.hpp>
#include "elf.hpp"



typedef void (*UserSpaceEntryPoint)();
extern "C" void JumpToUserMode(UserSpaceEntryPoint entryPoint, const void* userArgs, const void* userStack);


void usermode_init()
{
    // TODO: Temp hack until we have proper VDSO
    const auto vmaOffset = ((uintptr_t)&g_vdso) - (uintptr_t)VMA_VDSO_START;
    g_vdso.syscall -= vmaOffset;
    g_vdso.syscall_exit -= vmaOffset;
}


static void usermode_entry_spawn(Task* task, const Module* module)
{
    //Log("User module at %X, size is %X\n", module->address, module->size);

    const physaddr_t entry = elf_map(&task->m_pageTable, module->address, module->size);
    if (!entry)
    {
        Fatal("Could not load / start user process\n");
    }

    //Log("Module entry point at %X\n", entry);

    // TODO: dynamically allocate the stack location (at top of heap) instead of using constants?
    //       it would do the same thing, but less code...?
    task->m_userStackTop = VMA_USER_STACK_START;
    task->m_userStackBottom = VMA_USER_STACK_END;

    g_bigKernelLock.unlock();

    JumpToUserMode((UserSpaceEntryPoint)entry, nullptr, (void*)task->m_userStackBottom);
}


void usermode_spawn(const Module* module)
{
    new Task(usermode_entry_spawn, module, 0);
}


struct UserCloneContext
{
    const void* entry;
    const void* args;
    int flags;
    const void* userStack;
    size_t userStackSize;
};


static void usermode_entry_clone(Task* task, UserCloneContext* context)
{
    const auto entry = context->entry;
    const auto args = context->args;
    //const auto flags = context->flags;
    const auto userStack = context->userStack;
    const auto userStackSize = context->userStackSize;

    //Log("User task entry at %p, arg %p, stack at %p\n", entry, args, userStack);

    // TODO: args needs to be passed to the user entry point
    task->m_userStackTop = const_cast<void*>(advance_pointer(userStack, -userStackSize));
    task->m_userStackBottom = const_cast<void*>(userStack);

    g_bigKernelLock.unlock();

    JumpToUserMode((UserSpaceEntryPoint)entry, args, userStack);
}


void usermode_clone(const void* userFunction, const void* userArgs, int userFlags, const void* userStack, size_t userStackSize)
{
    UserCloneContext context;
    context.entry = userFunction;
    context.args = userArgs;
    context.flags = userFlags;
    context.userStack = userStack;
    context.userStackSize = userStackSize;

    new Task(usermode_entry_clone, context, Task::CREATE_SHARE_PAGE_TABLE);
}
