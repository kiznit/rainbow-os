/*
    Copyright (c) 2023, Thierry Tremblay
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

#pragma once

#include <type_traits>

class Cpu;
class Task;

// Per-CPU data, accessed using %gs
struct CpuData
{
    Task* task{};
    Cpu* cpu{};
};

// x86 doesn't need any task data as everything can be stored in CpuData and accessed with %gs.
struct TaskData
{
};

// Read data for the current CPU
#define CPU_GET_DATA(FIELD)                                                                                                        \
    ({                                                                                                                             \
        std::remove_const<typeof(CpuData::FIELD)>::type result;                                                                    \
        asm("mov %%gs:%1, %0" : "=r"(result) : "m"(*(typeof(CpuData::FIELD)*)offsetof(CpuData, FIELD)));                           \
        result;                                                                                                                    \
    })

// Write data for the current CPU
#define CPU_SET_DATA(FIELD, value)                                                                                                 \
    ({ asm("mov %0, %%gs:%1" : : "r"(value), "m"(*(typeof(CpuData::FIELD)*)offsetof(CpuData, FIELD))); })

// Get / set the current task
inline Task* CpuGetTask()
{
    return CPU_GET_DATA(task);
}

inline void CpuSetTask(Task* task)
{
    CPU_SET_DATA(task, task);
}
