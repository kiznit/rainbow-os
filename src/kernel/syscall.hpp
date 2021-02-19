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

#ifndef _RAINBOW_KERNEL_SYSCALL_HPP
#define _RAINBOW_KERNEL_SYSCALL_HPP

#include <metal/cpu.hpp>
#include <kernel/task.hpp>
#include <kernel/x86/cpu.hpp>

struct _UserTask;

long syscall_exit(long status);
long syscall_mmap(const void* address, unsigned long length);
long syscall_munmap(void* address, unsigned long length);
long syscall_thread(const void* userFunction, const void* userArgs, unsigned long userFlags, const void* userStack, unsigned long userStackSize);
long syscall_ipc(ipc_endpoint_t destination, ipc_endpoint_t waitFrom, const void* sendBuffer, long lenSendBuffer, void* recvBuffer, long lenRecvBuffer);
long syscall_log(const char* text, unsigned long length);
long syscall_yield();
long syscall_init_user_tcb(pthread_t userTask);
long syscall_futex_wait(std::atomic_int* futex, long value);
long syscall_futex_wake(std::atomic_int* futex, long count);


class SyscallGuard
{
public:
    SyscallGuard()
    {
        // Save user space FPU state
        auto task = cpu_get_data(task);
        fpu_save(&task->m_fpuState);

        // TODO: do we need to reinitialize the FPU in any way? Perhaps control words?
    }

    ~SyscallGuard()
    {
        // Restore user space FPU state
        auto task = cpu_get_data(task);
        fpu_restore(&task->m_fpuState);
    }
};


#define SYSCALL_GUARD() SyscallGuard syscallGuard


#endif
