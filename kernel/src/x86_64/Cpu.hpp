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

#include "CpuData.hpp"
#include "InterruptTable.hpp"
#include "devices/Apic.hpp"

// Order of values is determined by syscall/sysret requirements.
enum class CpuSelector : uint16_t
{
    Null = 0x00,
    KernelCode = 0x08,
    KernelData = 0x10,
    UserCode = 0x23,
    UserData = 0x1b,
    Tss = 0x28
};

// Initialize the current CPU
void CpuInitialize();

// Get / set the current task. The current ask will be nullptr until the processor is bootstrapped.
inline Task* CpuGetTask()
{
    return CPU_GET_DATA(task);
}

inline void CpuSetTask(Task* task)
{
    CPU_SET_DATA(task, task);
}

// Get/set the local apic. Every APIC is at the same physical address.
Apic* CpuGetApic();
void CpuSetApic(std::unique_ptr<Apic> apic);
