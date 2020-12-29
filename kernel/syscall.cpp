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

#include <rainbow/syscall.h>
#include <cassert>
#include <kernel/biglock.hpp>
#include <kernel/kernel.hpp>
#include <kernel/usermode.hpp>


// TODO: make sure syscall_table is in read-only memory
const void* syscall_table[] =
{
    (void*)syscall_exit,
    (void*)syscall_mmap,
    (void*)syscall_munmap,
    (void*)syscall_thread,
    (void*)syscall_ipc,
    (void*)syscall_log,
    (void*)syscall_yield
};



int syscall_exit()
{
    assert(!interrupt_enabled());

    BIG_KERNEL_LOCK();

    // TODO
    for (;;);
    return -1;
}


int syscall_mmap(const void* address, uintptr_t length)
{
    assert(!interrupt_enabled());

    BIG_KERNEL_LOCK();

    const auto pageCount = align_up(length, MEMORY_PAGE_SIZE) >> MEMORY_PAGE_SHIFT;

    const auto task = cpu_get_data(task);

    // TODO: allocating continuous frames might fail, need better API
    auto frame = pmm_allocate_frames(pageCount);
    task->pageTable.MapPages(frame, address, pageCount, PAGE_PRESENT | PAGE_USER | PAGE_WRITE | PAGE_NX);

    return (intptr_t)address;
}


int syscall_munmap(uintptr_t address, uintptr_t length)
{
    assert(!interrupt_enabled());

    BIG_KERNEL_LOCK();

    (void)address;
    (void)length;

    // TODO: parameter validation, handling flags, etc
    //const auto pageCount = align_up(length, MEMORY_PAGE_SIZE) >> MEMORY_PAGE_SHIFT;
    // TODO: vmm_free_pages(pageCount);
    return 0;
}


int syscall_thread(const void* userFunction, const void* userArgs, uintptr_t userFlags, const void* userStack, uintptr_t userStackSize)
{
    assert(!interrupt_enabled());

    BIG_KERNEL_LOCK();

    // TODO: parameter validation, handling flags, etc

    // Log("SYSCALL_THREAD:\n");
    // Log("    userFunction: %p\n", userFunction);
    // Log("    userArgs    : %p\n", userArgs);
    // Log("    userFlags   : %p\n", userFlags);
    // Log("    userStack   : %p\n", userStack);
    return usermode_clone(userFunction, userArgs, userFlags, userStack, userStackSize);
}


int syscall_log(const char* text)
{
    assert(!interrupt_enabled());

    BIG_KERNEL_LOCK();

    Log(text);
    return 0;
}


int syscall_yield()
{
    assert(!interrupt_enabled());

    BIG_KERNEL_LOCK();

    sched_yield();
    return 0;
}
