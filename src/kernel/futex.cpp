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

#include <cerrno>
#include <climits>
#include <kernel/biglock.hpp>
#include <kernel/syscall.hpp>
#include <kernel/vmm.hpp>
#include <kernel/waitqueue.hpp>


static_assert(sizeof(int) == sizeof(std::atomic_int));

// TODO: need a hash table...
static physaddr_t s_futexAddresses[100];
static WaitQueue  s_futexQueues[100];




long syscall_futex_wait(std::atomic_int* futex, long value)
{
    BIG_KERNEL_LOCK();
    SYSCALL_GUARD();

    if (futex->load(std::memory_order_acquire) == value)
    {
        // TODO: validate 'address'
        const auto address = vmm_get_physical_address(futex);

        int queue = -1;
        int free = -1;

        for (int i = 0; i != std::ssize(s_futexAddresses); ++i)
        {
            if (s_futexAddresses[i] == address)
            {
                queue = i;
                break;
            }
            else if (s_futexAddresses[i] == 0 && free < 0)
            {
                free = i;
            }
        }

        if (queue < 0) queue = free;
        assert(queue >= 0); // out of entries!

        s_futexAddresses[queue] = address;
        // TODO: are we suffering from the lost wake-up problem here?
        s_futexQueues[queue].Suspend(TaskState::Futex);
    }
    else
    {
        return EAGAIN;
    }

    return 0;
}


long syscall_futex_wake(std::atomic_int* futex, long count)
{
    BIG_KERNEL_LOCK();
    SYSCALL_GUARD();

    // TODO: validate 'address'
    const auto address = vmm_get_physical_address(futex);

    int result = 0;

    for (int i = 0; i != std::ssize(s_futexAddresses); ++i)
    {
        if (s_futexAddresses[i] == address)
        {
            if (count != INT_MAX)
            {
                result = s_futexQueues[i].Wakeup(count);
            }
            else
            {
                result = s_futexQueues[i].WakeupAll();
            }

            // TODO: if the queue is empty, we should delete it at some point (perhaps when user process dies?)

            break;
        }
    }

    return result;
}
