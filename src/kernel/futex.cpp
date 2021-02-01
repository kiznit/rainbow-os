/*
    Copyright (c) 2021, Thierry Tremblay
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

#include "futex.hpp"
#include <unordered_map>
#include <kernel/biglock.hpp>
#include <kernel/syscall.hpp>
#include <kernel/task.hpp>
#include <kernel/waitqueue.hpp>


static_assert(sizeof(int) == sizeof(std::atomic_int));

static std::unordered_map<physaddr_t,WaitQueue> g_futexQueues;


int syscall_futex_wait(std::atomic_int* futex, int value) noexcept
{
    SYSCALL_ENTER();
    BIG_KERNEL_LOCK();
    SYSCALL_GUARD();

    if (futex->load(std::memory_order_acquire) == value)
    {
        auto task = cpu_get_data(task);
        const auto address = task->m_pageTable->GetPhysicalAddress(futex);

        const auto [it, inserted] = g_futexQueues.try_emplace(address);
        // TODO: are we suffering from the lost wake-up problem here?
        it->second.Suspend(TaskState::Futex);
    }
    else
    {
        return EWOULDBLOCK;
    }

    SYSCALL_LEAVE(0);
}


int syscall_futex_wake(std::atomic_int* futex) noexcept
{
    SYSCALL_ENTER();
    BIG_KERNEL_LOCK();
    SYSCALL_GUARD();

    auto task = cpu_get_data(task);
    const auto address = task->m_pageTable->GetPhysicalAddress(futex);

    const auto it = g_futexQueues.find(address);
    if (it != g_futexQueues.end())
    {
        it->second.WakeupOne();

        // TODO: if the queue is empty, we should delete it at some point (perhaps when user process dies?)
    }

    SYSCALL_LEAVE(0);
}
