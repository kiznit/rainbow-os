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
#include "Task.hpp"
#include "devices/GicCpuInterface.hpp"

class Cpu
{
public:
    Cpu() = default;

    Cpu(const Cpu&) = delete;
    Cpu& operator=(const Cpu&) = delete;

    void Initialize();

    static Cpu& GetCurrent() { return *GetCurrentTask()->cpu_; }

    // Get / set the current task
    static Task* GetCurrentTask() { return reinterpret_cast<Task*>(mtl::Read_TPIDR_EL1()); }
    static void SetCurrentTask(Task* task)
    {
        task->cpu_ = GetCurrentTask()->cpu_;
        mtl::Write_TPIDR_EL1(reinterpret_cast<uintptr_t>(task));
    }

    // Get/set the GICC, if any. These are statics because every GICC is at the same physical address.
    // Retrieving the GICC for a different CPU than the current one wouldn't work as changes to it would
    // end up changing the current CPU's GICC instead of the intended one. To try to prevent this, we store
    // the GICC using unique_ptr.
    static GicCpuInterface* GetGicCpuInterface() { return GetCurrent().m_gicc.get(); }
    static void SetGicCpuInterface(std::unique_ptr<GicCpuInterface> gicc) { GetCurrent().m_gicc = std::move(gicc); }

private:
    TaskData m_initData;
    std::unique_ptr<GicCpuInterface> m_gicc;
};
