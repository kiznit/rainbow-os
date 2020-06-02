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

#ifndef _RAINBOW_RAINBOW_H
#define _RAINBOW_RAINBOW_H

#include <string.h>
#include <sys/types.h>
#include "syscall.h"


#if defined(__i386__)
#include <rainbow/arch/ia32/syscall.h>
#elif defined(__x86_64__)
#include <rainbow/arch/x86_64/syscall.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif


// Linux:
//  void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
//  int munmap(void *addr, size_t length);

// Windows:
//  LPVOID VirtualAlloc(LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect);
//  BOOL VirtualFree(LPVOID lpAddress, SIZE_T dwSize, DWORD dwFreeType);

static inline void* mmap(void* address, size_t length)
{
    return (void*)syscall2(SYSCALL_MMAP, (intptr_t)address, length);
}


static inline int munmap(void* address, size_t length)
{
  return syscall2(SYSCALL_MUNMAP, (intptr_t)address, length);
}


static inline int spawn(int (*function)(void*), const void* args, int flags, const void* stack, size_t stackSize)
{
    return syscall5(SYSCALL_THREAD, (intptr_t)function, (intptr_t)args, flags, (intptr_t)stack, stackSize);
}


// Send a message to a service. This is a blocking call.
// Any data returned by the service will go into the buffer.
static inline int ipc_call(pid_t destination, const void* sendBuffer, int lenSendBuffer, void* recvBuffer, int lenRecvBuffer)
{
    return syscall6(SYSCALL_IPC, destination, destination, (intptr_t)sendBuffer, lenSendBuffer, (intptr_t)recvBuffer, lenRecvBuffer);
}


// Wait for a call from a specific client. This is a blocking call.
// static inline int ipc_receive(pid_t from, void* recvBuffer, int lenRecvBuffer)
// {
//     return syscall6(SYSCALL_IPC, 0, from, 0, 0, (intptr_t)recvBuffer, lenRecvBuffer);
// }


// Reply to destination with the specified message and wait for the next one. This is a blocking call.
// This is basically ipc_send() + ipc_wait() in one call.
static inline int ipc_reply_and_wait(int destination, const void* sendBuffer, int lenSendBuffer, void* recvBuffer, int lenRecvBuffer)
{
    return syscall6(SYSCALL_IPC, destination, -1, (intptr_t)sendBuffer, lenSendBuffer, (intptr_t)recvBuffer, lenRecvBuffer);
}


// Send a message to a service. This is a blocking call.
// static inline int ipc_send(pid_t destination, const void* sendBuffer, int lenSendBuffer)
// {
//     return syscall6(SYSCALL_IPC, destination, 0, (intptr_t)sendBuffer, lenSendBuffer, 0, 0);
// }


// Wait for a call from any client. This is a blocking call.
static inline int ipc_wait(void* recvBuffer, int lenRecvBuffer)
{
    return syscall6(SYSCALL_IPC, 0, -1, 0, 0, (intptr_t)recvBuffer, lenRecvBuffer);
}



#ifdef __cplusplus
}
#endif

#endif
