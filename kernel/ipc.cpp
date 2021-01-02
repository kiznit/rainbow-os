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

#include <cstring>
#include <kernel/biglock.hpp>
#include <kernel/kernel.hpp>
#include <rainbow/ipc.h>
#include "waitqueue.inl"

// TODO: is this the right place / design?
static WaitQueue s_ipcReceivers;  // List of tasks blocked on receive phase


int syscall_ipc(ipc_endpoint_t sendTo, ipc_endpoint_t receiveFrom, const void* sendBuffer, int lenSendBuffer, void* recvBuffer, int lenRecvBuffer)
{
    assert(!interrupt_enabled());

    BIG_KERNEL_LOCK();
    SYSCALL_GUARD();

    // TODO: parameters validation!

#if defined(__i386__)
    lenRecvBuffer = *(int*)lenRecvBuffer;
#endif

    auto current = cpu_get_data(task);
    Task* receiver = nullptr;

    //Log("%d syscall_ipc(%d, %d, %p, %d, %p, %d)\n", current->id, sendTo, receiveFrom, sendBuffer, lenSendBuffer, recvBuffer, lenRecvBuffer);

    // TODO: virtual registers should be accessible and filled in user space
    memcpy(current->ipcRegisters, sendBuffer, min<int>(lenSendBuffer, sizeof(Task::ipcRegisters)));

    // Send phase
    if (sendTo != IPC_ENDPOINT_NONE)
    {
        receiver = Task::Get(sendTo);
        if (!receiver)
        {
            Log("IPC: receiver %d not found\n", sendTo);
            return -1;
        }

        if (current == receiver)
        {
            Log("IPC: sender and receiver are the same (%d)\n", sendTo);
            return -1;
        }

        // Is the receiver waiting and ready and willing to receive us?
        if (receiver->state == Task::STATE_IPC_RECEIVE && (receiver->ipcPartner == IPC_ENDPOINT_ANY || receiver->ipcPartner == current->id))
        {
            // Great, nothing to do here!
            //Log("%d IPC: receiver %d is ready\n", current->id, receiver->id);
        }
        else
        {
            // Receiver is not ready, block and wait
            //Log("%d IPC: receiver %d not ready (state %d), blocking\n", current->id, receiver->id, receiver->state);
            current->ipcPartner = receiver->id;
            sched_suspend(receiver->ipcSenders, Task::STATE_IPC_SEND); // TODO: do we want to yield CPU to receiver?
        }

        // Transfer message
        receiver->ipcPartner = current->id;

        assert(current->state == Task::STATE_RUNNING);
        assert(receiver->state == Task::STATE_IPC_RECEIVE);

        memcpy(receiver->ipcRegisters, current->ipcRegisters, sizeof(Task::ipcRegisters));

        sched_wakeup(receiver);
    }

    // Receive phase
    int result = 0;

    if (receiveFrom != IPC_ENDPOINT_NONE)
    {
        Task* sender = nullptr;

        if (receiveFrom == IPC_ENDPOINT_ANY)
        {
            // Open wait
            sender = current->ipcSenders.front();
            // if (sender)
            //     Log("%d IPC: open wait accepting sender %d\n", current->id, sender->id);
            // else
            //     Log("%d IPC: open wait - no sender\n", current->id);
        }
        else
        {
            // Closed wait
            sender = Task::Get(receiveFrom);
            //Log("%d IPC: closed wait accepting sender %d (state %d)\n", current->id, sender->id, sender->state);
        }

        if (sender == nullptr || sender->ipcPartner != current->id || sender->state != Task::STATE_IPC_SEND)
        {
            // We don't have a partner, block until we do
            current->ipcPartner = receiveFrom;
            //Log("%d IPC: sender is %d, blocking and waiting for %d\n", current->id, sender ? sender->id : 0, current->ipcPartner);
            sched_suspend(s_ipcReceivers, Task::STATE_IPC_RECEIVE);
        }
        else
        {
            // We have a partner ready to send
            assert(sender->state == Task::STATE_IPC_SEND || sender->state == Task::STATE_IPC_RECEIVE);
            current->ipcPartner = sender->id;
            sched_wakeup(sender);
            //Log("%d IPC: partner ready, suspending and switching to sender %d (state %d)\n", current->id, sender->id, sender->state);
            sched_suspend(s_ipcReceivers, Task::STATE_IPC_RECEIVE, sender);
        }

        result = current->ipcPartner;
    }

    // TODO: virtual registers should be accessible and filled in user space
    memcpy(recvBuffer, current->ipcRegisters, min<int>(lenRecvBuffer, sizeof(Task::ipcRegisters)));

    return result;
}
