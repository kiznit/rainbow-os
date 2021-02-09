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

#ifndef _RAINBOW_IPC_H
#define _RAINBOW_IPC_H

#include <rainbow/syscall.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int ipc_endpoint_t;

#define IPC_ENDPOINT_NONE 0
#define IPC_ENDPOINT_ANY  (-1)


// Send a message and wait for a reply. This emulates a function call.
// This is a blocking call.
static inline int ipc_call(ipc_endpoint_t sendTo, const void* sendBuffer, int lenSendBuffer, void* recvBuffer, int lenRecvBuffer)
{
    return __syscall6(SYSCALL_IPC, sendTo, sendTo, (intptr_t)sendBuffer, lenSendBuffer, (intptr_t)recvBuffer, lenRecvBuffer);
}


// Wait for a message from a specific source.
// This is a blocking call.
static inline int ipc_receive(ipc_endpoint_t receiveFrom, void* recvBuffer, int lenRecvBuffer)
{
    return __syscall6(SYSCALL_IPC, IPC_ENDPOINT_NONE, receiveFrom, 0, 0, (intptr_t)recvBuffer, lenRecvBuffer);
}


// Reply to a caller with a message and wait for a message from any source.
// This is basically ipc_send() + ipc_wait() in one call.
// This is a blocking call.
static inline int ipc_reply_and_wait(ipc_endpoint_t sendTo, const void* sendBuffer, int lenSendBuffer, void* recvBuffer, int lenRecvBuffer)
{
    return __syscall6(SYSCALL_IPC, sendTo, IPC_ENDPOINT_ANY, (intptr_t)sendBuffer, lenSendBuffer, (intptr_t)recvBuffer, lenRecvBuffer);
}


// Send a message..
// This is a blocking call.
static inline int ipc_send(ipc_endpoint_t sendTo, const void* sendBuffer, int lenSendBuffer)
{
    return __syscall6(SYSCALL_IPC, sendTo, IPC_ENDPOINT_NONE, (intptr_t)sendBuffer, lenSendBuffer, 0, 0);
}


// Wait for a message from any source.
// This is a blocking call.
static inline int ipc_wait(void* recvBuffer, int lenRecvBuffer)
{
    return __syscall6(SYSCALL_IPC, IPC_ENDPOINT_NONE, IPC_ENDPOINT_ANY, 0, 0, (intptr_t)recvBuffer, lenRecvBuffer);
}



#ifdef __cplusplus
}
#endif

#endif
