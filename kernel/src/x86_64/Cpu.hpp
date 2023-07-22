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

#include "interrupt.hpp"
#include <memory>

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

class Cpu;
class Task;

// Data accessed using %gs
struct GsData
{
    Task* task{};
    Cpu* cpu{};
};

// Read data for the current CPU
#define GS_GET_DATA(FIELD)                                                                                                         \
    ({                                                                                                                             \
        std::remove_const<typeof(GsData::FIELD)>::type result;                                                                     \
        asm("mov %%gs:%1, %0" : "=r"(result) : "m"(*(typeof(GsData::FIELD)*)offsetof(GsData, FIELD)));                             \
        result;                                                                                                                    \
    })

// Write data for the current CPU
#define GS_SET_DATA(FIELD, value)                                                                                                  \
    ({ asm("mov %0, %%gs:%1" : : "r"(value), "m"(*(typeof(GsData::FIELD)*)offsetof(GsData, FIELD))); })

class Cpu
{
public:
    Cpu();

    Cpu(const Cpu&) = delete;
    Cpu& operator=(const Cpu&) = delete;

    void Initialize();

    static Cpu& GetCurrent() { return *GS_GET_DATA(cpu); }

    void SetTask(std::shared_ptr<Task> task)
    {
        m_task = std::move(task);
        GS_SET_DATA(task, m_task.get());
    }

private:
    void InitGdt();
    void InitTss();

    void LoadGdt();
    void LoadTss();

    InterruptTable m_idt;
    mtl::GdtDescriptor m_gdt[7];
    mtl::Tss m_tss;
    GsData m_gsData;
    std::shared_ptr<Task> m_task;
};
