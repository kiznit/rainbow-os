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

#include "syscall.hpp"
#include <cerrno>
#include <kernel/biglock.hpp>
#include <kernel/scheduler.hpp>
#include <kernel/usermode.hpp>
#include <metal/helpers.hpp>
#include <metal/log.hpp>
#include <kernel/x86/selectors.hpp>

extern Scheduler g_scheduler;


// TODO: make sure syscall_table is in read-only memory
const void* syscall_table[] =
{
    (void*)syscall_exit,
    (void*)syscall_mmap,
    (void*)syscall_munmap,
    (void*)syscall_thread,
    (void*)syscall_ipc,
    (void*)syscall_log,
    (void*)syscall_yield,
    (void*)syscall_init_user_tcb,
    (void*)syscall_futex_wait,
    (void*)syscall_futex_wake,
};



intptr_t syscall_exit(intptr_t status)
{
    BIG_KERNEL_LOCK();
    SYSCALL_GUARD();

    g_scheduler.Die(status);

    // Should never be reached
    for (;;);
}


intptr_t syscall_mmap(const void* address, uintptr_t length)
{
    BIG_KERNEL_LOCK();
    SYSCALL_GUARD();

    if (length == 0)
    {
        return -EINVAL;
    }

    if (address)
    {
        // TODO: implement address != nullptr
        return -EINVAL;
    }


    const auto task = cpu_get_data(task);
    const auto pageCount = align_up(length, MEMORY_PAGE_SIZE) >> MEMORY_PAGE_SHIFT;

    address = task->m_pageTable->AllocateUserPages(pageCount);
    if (!address)
    {
        return -ENOMEM;
    }

    return (intptr_t)address;
}


intptr_t syscall_munmap(void* address, uintptr_t length)
{
    BIG_KERNEL_LOCK();
    SYSCALL_GUARD();

    if (length == 0)
    {
        return EINVAL;
    }

    if (!is_aligned(address, MEMORY_PAGE_SIZE))
    {
        return EINVAL;
    }

    const auto task = cpu_get_data(task);
    const auto pageCount = align_up(length, MEMORY_PAGE_SIZE) >> MEMORY_PAGE_SHIFT;

    return task->m_pageTable->FreeUserPages(address, pageCount);
}


intptr_t syscall_thread(const void* userFunction, const void* userArgs, uintptr_t userFlags, const void* userStack, uintptr_t userStackSize)
{
    BIG_KERNEL_LOCK();
    SYSCALL_GUARD();

    // TODO: parameter validation, handling flags, etc

    // Log("SYSCALL_THREAD:\n");
    // Log("    userFunction: %p\n", userFunction);
    // Log("    userArgs    : %p\n", userArgs);
    // Log("    userFlags   : %p\n", userFlags);
    // Log("    userStack   : %p\n", userStack);
    usermode_clone(userFunction, userArgs, userFlags, userStack, userStackSize);

    return 0;
}


intptr_t syscall_log(const char* text, uintptr_t length)
{
    BIG_KERNEL_LOCK();
    SYSCALL_GUARD();

    console_print(text, length);

    return length;
}


intptr_t syscall_yield()
{
    BIG_KERNEL_LOCK();
    SYSCALL_GUARD();

    g_scheduler.Yield();

    return 0;
}


intptr_t syscall_init_user_tcb(pthread_t userTask)
{
    BIG_KERNEL_LOCK();
    SYSCALL_GUARD();

    // TODO: validate that *userTask is all in user space

    const auto task = cpu_get_data(task);

    userTask->self = userTask;
    userTask->id = task->m_id;

    task->m_userTask = userTask;

#if defined(__i386__)
    auto gdt = cpu_get_data(gdt);
    gdt[7].SetUserData32((uintptr_t)userTask, sizeof(pthread_t));
    asm volatile ("movl %0, %%gs\n" : : "r" (GDT_TLS) : "memory" );
#elif defined(__x86_64__)
    x86_write_msr(MSR_FS_BASE, (uintptr_t)userTask);
#endif

    return 0;
}
