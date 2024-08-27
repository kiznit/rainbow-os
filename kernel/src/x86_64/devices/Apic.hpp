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

#include "ErrorCode.hpp"
#include <cstdint>
#include <metal/expected.hpp>
#include <metal/helpers.hpp>

#define RESERVED(COUNT) uint32_t MTL_CONCAT(reserved_, __LINE__)[COUNT * 4];
#define REGISTER(NAME)                                                                                                             \
    uint32_t NAME;                                                                                                                 \
    uint32_t MTL_CONCAT(reserved_, __LINE__)[3];

// Advanced Programmable Interrupt Controller (APIC)
class Apic
{
public:
    explicit Apic(void* address);

    // Initialize the interrupt controller
    mtl::expected<void, ErrorCode> Initialize();

    // Accessors for the current CPU
    constexpr int GetId() const { return (m_registers->id >> 24) & 0xFF; }
    constexpr int GetInterruptCount() const { return ((m_registers->version >> 16) & 0xFF) + 1; }
    constexpr int GetVersion() const { return m_registers->version & 0xFF; }

    void EndOfInterrupt() { m_registers->eoi = 0; }

    static bool IsSpurious(int interrupt) { return interrupt == kSpuriousInterrupt; }

private:
    static constexpr auto kSpuriousInterrupt = 0xFF;

    struct Registers
    {
        RESERVED(2);
        REGISTER(id);
        REGISTER(version);
        RESERVED(4);
        REGISTER(taskPriority);
        REGISTER(arbitrationPriority);
        REGISTER(processorPriority);
        REGISTER(eoi);
        REGISTER(remoteRead);
        REGISTER(logicalDestination);
        REGISTER(destinationFormat);
        REGISTER(spuriousInterruptVector);
        // In-Service Register
        REGISTER(ISR0);
        REGISTER(ISR1);
        REGISTER(ISR2);
        REGISTER(ISR3);
        REGISTER(ISR4);
        REGISTER(ISR5);
        REGISTER(ISR6);
        REGISTER(ISR7);
        // Trigger Mode Register
        REGISTER(TMR0);
        REGISTER(TMR1);
        REGISTER(TMR2);
        REGISTER(TMR3);
        REGISTER(TMR4);
        REGISTER(TMR5);
        REGISTER(TMR6);
        REGISTER(TMR7);
        // Interrupt Request Register
        REGISTER(IRR0);
        REGISTER(IRR1);
        REGISTER(IRR2);
        REGISTER(IRR3);
        REGISTER(IRR4);
        REGISTER(IRR5);
        REGISTER(IRR6);
        REGISTER(IRR7);
        REGISTER(errorStatus);
        RESERVED(6);
        REGISTER(correctedMachineCheckErrorInterrupt);
        // Interrupt Command Register
        REGISTER(ICR0);
        REGISTER(ICR1);
        REGISTER(timer);
        REGISTER(thermalSensor);
        REGISTER(performanceMonitoringCounters);
        REGISTER(LINT0);
        REGISTER(LINT1);
        REGISTER(error);
        REGISTER(initialCount);
        REGISTER(currentCount);
        RESERVED(4);
        REGISTER(divideConfiguration);
        RESERVED(1);
    };

    static_assert(offsetof(Registers, id) == 0x20);
    static_assert(offsetof(Registers, version) == 0x30);
    static_assert(sizeof(Registers) == 0x400);

    volatile Registers* const m_registers;
};

#undef RESERVED
#undef REGISTER
