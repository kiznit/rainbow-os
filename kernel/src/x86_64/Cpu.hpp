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
#include "devices/Apic.hpp"
#include "interrupt.hpp"

// Order is determined by syscall/sysret requirements
enum class Selector : uint16_t
{
    Null = 0x00,
    KernelCode = 0x08,
    KernelData = 0x10,
    UserCode = 0x23,
    UserData = 0x1b,
    Tss = 0x28
};

class Cpu
{
public:
    Cpu();

    Cpu(const Cpu&) = delete;
    Cpu& operator=(const Cpu&) = delete;

    void Initialize();

    static Cpu& GetCurrent() { return *CPU_GET_DATA(cpu); }

    // Get / set the current task
    static Task* GetCurrentTask() { return CPU_GET_DATA(task); }
    static void SetCurrentTask(Task* task) { CPU_SET_DATA(task, task); }

    // Get/set the local apic, if any. These are statics because every APIC is at the same physical address.
    // Retrieving the APIC for a different CPU than the current one wouldn't work as changes to it would
    // end up changing the current CPU's APIC instead of the intended one. To try to prevent this, we store
    // the APIC using unique_ptr.
    static Apic* GetApic() { return GetCurrent().m_apic.get(); }
    static void SetApic(std::unique_ptr<Apic> apic) { GetCurrent().m_apic = std::move(apic); }

private:
    void InitGdt();
    void InitTss();

    void LoadGdt();
    void LoadTss();

    InterruptTable m_idt;
    mtl::GdtDescriptor m_gdt[7];
    mtl::Tss m_tss;
    CpuData m_cpuData;
    std::unique_ptr<Apic> m_apic;
};
