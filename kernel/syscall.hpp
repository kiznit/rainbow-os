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

#include <sys/types.h>


extern "C"
{
    int syscall_exit();
    int syscall_mmap(uintptr_t address, uintptr_t length);
    int syscall_munmap(uintptr_t address, uintptr_t length);
    int syscall_thread(const void* userFunction, const void* userArgs, uintptr_t userFlags, const void* userStack, uintptr_t userStackSize);
    int syscall_ipc_call(pid_t target, const void* message, int lenMessage, void* buffer, int lenBuffer);
    int syscall_ipc_reply(int caller, const void* message, int lenMessage);
    int syscall_ipc_reply_and_wait(int caller, const void* message, int lenMessage, void* buffer, int lenBuffer);
    int syscall_ipc_wait(void* buffer, int length);
    int syscall_log(const char* text);
}


#endif
