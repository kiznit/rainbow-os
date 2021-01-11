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

#include <algorithm>
#include <cassert>
#include <cstring>
#include <kernel/biglock.hpp>
#include <kernel/kernel.hpp>
#include <rainbow/ipc.h>

// TODO: is this the right place / design?
static WaitQueue s_ipcReceivers;  // List of tasks blocked on receive phase


int syscall_ipc(ipc_endpoint_t sendTo, ipc_endpoint_t receiveFrom, const void* sendBuffer, int lenSendBuffer, void* recvBuffer, int lenRecvBuffer) noexcept
{
    SYSCALL_ENTER();
    BIG_KERNEL_LOCK();
    SYSCALL_GUARD();

    // TODO: parameters validation!

#if defined(__i386__)
    lenRecvBuffer = *(int*)lenRecvBuffer;
#endif

    auto current = cpu_get_data(task);
    Task* receiver = nullptr;

    //Log("%d syscall_ipc(%d, %d, %p, %d, %p, %d)\n", current->m_id, sendTo, receiveFrom, sendBuffer, lenSendBuffer, recvBuffer, lenRecvBuffer);

    // TODO: virtual registers should be accessible and filled in user space
    memcpy(current->m_ipcRegisters, sendBuffer, std::min<int>(lenSendBuffer, sizeof(Task::m_ipcRegisters)));

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
        if (receiver->m_state == Task::STATE_IPC_RECEIVE && (receiver->m_ipcPartner == IPC_ENDPOINT_ANY || receiver->m_ipcPartner == current->m_id))
        {
            // Great, nothing to do here!
            //Log("%d IPC: receiver %d is ready\n", current->m_id, receiver->m_id);
        }
        else
        {
            // Receiver is not ready, block and wait
            //Log("%d IPC: receiver %d not ready (state %d), blocking\n", current->m_id, receiver->m_id, receiver->m_state);
            current->m_ipcPartner = receiver->m_id;
            sched_suspend(receiver->m_ipcSenders, Task::STATE_IPC_SEND); // TODO: do we want to yield CPU to receiver?
        }

        // Transfer message
        receiver->m_ipcPartner = current->m_id;

        assert(current->m_state == Task::STATE_RUNNING);
        assert(receiver->m_state == Task::STATE_IPC_RECEIVE);

        memcpy(receiver->m_ipcRegisters, current->m_ipcRegisters, sizeof(Task::m_ipcRegisters));

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
            sender = current->m_ipcSenders.front();
            // if (sender)
            //     Log("%d IPC: open wait accepting sender %d\n", current->m_id, sender->m_id);
            // else
            //     Log("%d IPC: open wait - no sender\n", current->m_id);
        }
        else
        {
            // Closed wait
            sender = Task::Get(receiveFrom);
            //Log("%d IPC: closed wait accepting sender %d (state %d)\n", current->m_id, sender->m_id, sender->m_state);
        }

        if (sender == nullptr || sender->m_ipcPartner != current->m_id || sender->m_state != Task::STATE_IPC_SEND)
        {
            // We don't have a partner, block until we do
            current->m_ipcPartner = receiveFrom;
            //Log("%d IPC: sender is %d, blocking and waiting for %d\n", current->m_id, sender ? sender->m_id : 0, current->m_ipcPartner);
            sched_suspend(s_ipcReceivers, Task::STATE_IPC_RECEIVE);
        }
        else
        {
            // We have a partner ready to send
            assert(sender->m_state == Task::STATE_IPC_SEND || sender->m_state == Task::STATE_IPC_RECEIVE);
            current->m_ipcPartner = sender->m_id;
            sched_wakeup(sender);
            //Log("%d IPC: partner ready, suspending and switching to sender %d (state %d)\n", current->m_id, sender->m_id, sender->m_state);
            sched_suspend(s_ipcReceivers, Task::STATE_IPC_RECEIVE, sender);
        }

        result = current->m_ipcPartner;
    }

    // TODO: virtual registers should be accessible and filled in user space
    memcpy(recvBuffer, current->m_ipcRegisters, std::min<int>(lenRecvBuffer, sizeof(Task::m_ipcRegisters)));

    SYSCALL_EXIT(result);
}
