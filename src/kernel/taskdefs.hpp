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

#ifndef _RAINBOW_KERNEL_TASKDEFS_HPP
#define _RAINBOW_KERNEL_TASKDEFS_HPP


enum class TaskState
{
    Init,       // 0 - Task is initializing
    Running,    // 1 - Task is running
    Ready,      // 2 - Task is ready to run

    // Blocked states
    Sleep,      // 3 - Task is sleeping until 'm_sleepUntilNs'
    Zombie,     // 4 - Task died, but has not been destroyed / freed yet
    IpcSend,    // 5 - IPC send phase
    IpcReceive, // 6 - IPC receive phase
    Mutex,      // 7 - Task is blocked on a mutex
    Futex,      // 8 - Task is blocked on a futex
};


enum class TaskPriority
{
    Idle,       // Reserved for idle tasks, do not use if you want any CPU time
    Low,
    Normal,
    High,

    Count       // How many priority levels exist
};

constexpr auto TaskPriorityCount = static_cast<int>(TaskPriority::Count);


inline bool operator<(TaskPriority a, TaskPriority b)
{
    return static_cast<int>(a) < static_cast<int>(b);
}


#endif

