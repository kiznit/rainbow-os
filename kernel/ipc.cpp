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

#include <kernel/kernel.hpp>
#include "waitqueue.inl"


// TODO: It is redundant to have SYSCALL function numbers + IPC received id.
//       L4Ka keeps SYSENTER only for IPCs and uses INT for other system calls, which seems to
//       make sense if there are few system calls and they are seldom used.
//       I am not sure if the 64 bits version reserves SYSCALL for IPCs in the same way.

// TODO: use IPC endpoints (ports) instead of task ids to identify sources/destinations

extern "C" int syscall_ipc_call(pid_t destination, const void* message, int lenMessage, void* buffer, int lenBuffer)
{
    // TODO: parameters validation!

    auto receiver = Task::Get(destination);
    if (!receiver)
    {
        Log("syscall_ipc_call: destination %d not found\n", destination);
        return -1;
    }

    auto caller = cpu_get_data(task);
    if (caller == receiver)
    {
        Log("syscall_ipc_call: source and destination are the same (%d)\n", destination);
        return -1;
    }

    // TODO: virtual registers should be accessible and filled in user space
    memcpy(caller->ipcRegisters, message, min<int>(lenMessage, sizeof(Task::ipcRegisters)));

    if (receiver->state == Task::STATE_WAIT)
    {
        //Log("%d: syscall_ipc_call - waking up receiver %d\n", caller->id, receiver->id);
        // TODO: combine the next two calls in one?
        g_scheduler->Wakeup(receiver);
        g_scheduler->Suspend(receiver->ipcCallers, Task::STATE_CALL, receiver);
    }
    else
    {
        // Receiver not ready to service us, block until it is
        //Log("%d: syscall_ipc_call - suspending as receiver %d is not ready\n", caller->id, receiver->id);
        g_scheduler->Suspend(receiver->ipcCallers, Task::STATE_CALL);
    }

    //Log((char*)message);

    memcpy(buffer, caller->ipcRegisters, min<int>(lenBuffer, sizeof(Task::ipcRegisters)));

    return 0;
}


extern "C" int syscall_ipc_reply(int callerId, const void* message, int lenMessage)
{
    // TODO: parameters validation!

    auto caller = Task::Get(callerId);
    if (!caller)
    {
        Log("syscall_ipc_reply: caller %d not found\n", callerId);
        return -1;
    }

    if (caller->state != Task::STATE_REPLY)
    {
        Log("syscall_ipc_reply: caller %d not awaiting a reply\n", callerId);
        return -1;
    }

    auto service = cpu_get_data(task);
    if (caller == service)
    {
        Log("syscall_ipc_reply: caller and service are the same (%d)\n", callerId);
        return -1;
    }

    //Log("%d: syscall_ipc_reply: replying to %d\n", service->id, caller->id);

    memcpy(caller->ipcRegisters, message, min<int>(lenMessage, sizeof(Task::ipcRegisters)));

    g_scheduler->Wakeup(caller);

    return 0;
}



extern "C" int syscall_ipc_reply_and_wait(int callerId, const void* message, int lenMessage, void* buffer, int lenBuffer)
{
    // TODO: parameters validation!

    // TODO: ipc_reply_and_wait() look a lot like ipc_call(), can we merge the functionality?

    // TODO: optimize this so that the caller is waked up, the current thread put to sleep and execution switched to the caller (?)

    int result = syscall_ipc_reply(callerId, message, lenMessage);
    if (result < 0)
    {
        return result;
    }

    return syscall_ipc_wait(buffer, lenBuffer);
}



extern "C" int syscall_ipc_wait(void* buffer, int length)
{
    // TODO: parameters validation!

    auto service = cpu_get_data(task);
    assert(service->next == nullptr);

    // TODO: is this loop needed?
    while (service->ipcCallers.empty())
    {
        //Log("%d: syscall_ipc_wait: service suspending as there are no blocked callers\n", service->id);
        g_scheduler->Suspend(g_scheduler->m_ipcWaiters, Task::STATE_WAIT);
        //Log("%d: syscall_ipc_wait: service resuming\n", service->id);
    }

    auto caller = service->ipcCallers.front();

    // Copy message from caller to service
    memcpy(buffer, caller->ipcRegisters, min<int>(length, sizeof(Task::ipcRegisters)));

    // Switch caller from STATE_CALL to STATE_REPLY
    service->ipcCallers.remove(caller);
    caller->state = Task::STATE_REPLY;
    service->ipcWaitReply.push_back(caller);

    // Return the caller's id to the service. This will be used for replying and locate the caller.
    return caller->id;
}
