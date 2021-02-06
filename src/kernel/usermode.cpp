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

extern Scheduler g_scheduler;


typedef void (*UserSpaceEntryPoint)();
extern "C" void JumpToUserMode(UserSpaceEntryPoint entryPoint, const void* userArgs, const void* userStack);


void usermode_init()
{
    // TODO: Temp hack until we have proper VDSO
    const auto vmaOffset = ((uintptr_t)&g_vdso) - (uintptr_t)VMA_VDSO_START;
    g_vdso.syscall -= vmaOffset;
    g_vdso.syscall_exit -= vmaOffset;
}


#define AUX_VECTOR(id, value) *--p = (long)(value); *--p = (long)(id);

// Returns pointer to args, which happens to also be the top of user space stack
static void* build_aux_vectors(Task* task, const ElfImageInfo& info)
{
    auto p = (long*)task->m_userStackBottom;

    // ELF auxv
    AUX_VECTOR(AT_NULL, 0);
    AUX_VECTOR(AT_PAGESZ,   MEMORY_PAGE_SIZE);
    AUX_VECTOR(AT_ENTRY,    info.entry);
    AUX_VECTOR(AT_PHNUM,    info.phnum);
    AUX_VECTOR(AT_PHENT,    info.phent);
    AUX_VECTOR(AT_PHDR,     info.phdr);

    // Environment
    *--p = 0;

    // Program arguments
    *--p = 0;
    *--p = 0;   // argv - TODO: get the program name on the user stack and point to it
    *--p = 1;   // argc

    return p;
}


static void usermode_entry_spawn(Task* task, const Module* module)
{
    //Log("User module at %X, size is %X\n", module->address, module->size);

    ElfImageInfo info;

    elf_map(task, module->address, module->size, info);

    //Log("Module entry point at %p\n", info.entry);

    // Note: we can only initialize TLS when the task's page table is active
    task->InitUserTaskAndTls();

    auto args = build_aux_vectors(task, info);
    auto userStack = args;

#if defined(__x86_64__)
    // ABI requires us to properly align the stack
    if (!((uintptr_t)userStack & 0x15))
    {
        userStack = advance_pointer(userStack, -8);
    }
#endif

    g_bigKernelLock.unlock();

    JumpToUserMode((UserSpaceEntryPoint)info.entry, args, userStack);
}


void usermode_spawn(const Module* module)
{
    auto task = std::make_unique<Task>(usermode_entry_spawn, module, cpu_get_data(task)->m_pageTable->CloneKernelSpace());

    // TODO: dynamically allocate the stack location (at top of heap) instead of using constants?
    //       it would do the same thing, but less code...?
    task->m_userStackTop = VMA_USER_STACK_START;
    task->m_userStackBottom = VMA_USER_STACK_END;

    g_scheduler.AddTask(std::move(task));
}


struct UserCloneContext
{
    const void* entry;
    const void* args;
};


static void usermode_entry_clone(Task* task, UserCloneContext* context)
{
    const auto entry = context->entry;
    const auto args = context->args;

    //Log("User task entry at %p, arg %p, stack at %p\n", entry, args, task->m_userStackBottom);

    // Note: we can only initialize TLS when the task's page table is active
    task->InitUserTaskAndTls();

    const void* userStack = task->m_userStackBottom;

    g_bigKernelLock.unlock();
    JumpToUserMode((UserSpaceEntryPoint)entry, args, userStack);
}


void usermode_clone(const void* userFunction, const void* userArgs, int userFlags, const void* userStack, size_t userStackSize)
{
    (void)userFlags;

    UserCloneContext context;
    context.entry = userFunction;
    context.args = userArgs;

    auto currentTask = cpu_get_data(task);

    auto task = std::make_unique<Task>(usermode_entry_clone, context, currentTask->m_pageTable);

    // TODO: args needs to be passed to the user entry point
    task->m_userStackTop = const_cast<void*>(advance_pointer(userStack, -userStackSize));
    task->m_userStackBottom = const_cast<void*>(userStack);

    // TLS
    task->m_tlsTemplate = currentTask->m_tlsTemplate;
    task->m_tlsTemplateSize = currentTask->m_tlsTemplateSize;
    task->m_tlsSize = currentTask->m_tlsSize;

    g_scheduler.AddTask(std::move(task));
}
