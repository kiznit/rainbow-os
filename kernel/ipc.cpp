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

// TODO: is this the right place / design?
static WaitQueue s_ipcWaiters;  // List of tasks blocked on ipc_wait

#define STATE_IPC_REPLY STATE_IPC_WAIT

// TODO: It is redundant to have SYSCALL function numbers + IPC received id.
//       L4Ka keeps SYSENTER only for IPCs and uses INT for other system calls, which seems to
//       make sense if there are few system calls and they are seldom used.
//       I am not sure if the 64 bits version reserves SYSCALL for IPCs in the same way.

// TODO: use IPC endpoints (ports) instead of task ids to identify sources/destinations

// TODO: investigate using capabilities (EROS IPC)


/*
    TODO: design comments

    L4: uses timeout on send()

    --> ipc_call() should block
    --> ipc_reply() or ipc_send() should not! service can reply with a "do not wait" timeout

*/




int syscall_ipc(pid_t destination, pid_t waitFrom, const void* sendBuffer, int lenSendBuffer, void* recvBuffer, int lenRecvBuffer)
{
    // TODO: parameters validation!

#if defined(__i386__)
    // Fix arg6
    lenRecvBuffer = *(int*)lenRecvBuffer;
#endif

    auto task = cpu_get_data(task);

    //Log("%d: syscall_ipc: %d, %d, %p, %d, %p, %d\n", task->id, destination, waitFrom, sendBuffer, lenSendBuffer, recvBuffer, lenRecvBuffer);

    // TODO: virtual registers should be accessible and filled in user space
    memcpy(task->ipcRegisters, sendBuffer, min<int>(lenSendBuffer, sizeof(Task::ipcRegisters)));

    // Send phase
    if (destination)
    {
        auto receiver = Task::Get(destination);
        if (!receiver)
        {
            Log("syscall_ipc: destination %d not found\n", destination);
            return -1;
        }

        if (task == receiver)
        {
            Log("syscall_ipc: source and destination are the same (%d)\n", destination);
            return -1;
        }

        if (receiver->state == Task::STATE_IPC_WAIT)
        {
            //Log("%d: syscall_ipc - waking up receiver %d\n", task->id, receiver->id);
            sched_wakeup(receiver);
        }

        task->ipcWaitFrom = destination;
        sched_suspend(receiver->ipcCallers, Task::STATE_IPC_SEND, receiver);
    }

    // Receive phase
    int result = 0;

    if (waitFrom)
    {
        // TODO: handle open vs closed wait

        // TODO: is this loop needed?
        while (task->ipcCallers.empty())
        {
            //Log("%d: syscall_ipc: task suspending as there are no blocked callers\n", task->id);
            task->ipcWaitFrom = waitFrom;
            sched_suspend(s_ipcWaiters, Task::STATE_IPC_WAIT);
            //Log("%d: syscall_ipc: task resuming\n", task->id);
        }

        auto caller = task->ipcCallers.front();

        // Copy message from caller to task
        memcpy(task->ipcRegisters, caller->ipcRegisters, sizeof(Task::ipcRegisters));

        // Switch caller from STATE_IPC_SEND to STATE_IPC_WAIT
        task->ipcCallers.remove(caller);
        assert(caller->state == Task::STATE_IPC_SEND);
        assert(caller->ipcWaitFrom == task->id);
        caller->state = Task::STATE_IPC_WAIT;
        task->ipcWaitReply.push_back(caller);

        // Return the caller's id to the task. This will be used for replying and locate the caller.
        result = caller->id;
    }

    // TODO: virtual registers should be accessible and filled in user space
    memcpy(recvBuffer, task->ipcRegisters, min<int>(lenRecvBuffer, sizeof(Task::ipcRegisters)));

    return result;
}
