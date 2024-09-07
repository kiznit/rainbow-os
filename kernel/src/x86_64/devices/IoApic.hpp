/*
    Copyright (c) 2024, Thierry Tremblay
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

#include "acpi/Acpi.hpp"
#include "interfaces/IInterruptController.hpp"
#include <cstdint>

// 82093AA I/O Advanced Programmable Interrupt Controller (IOAPIC)
// Ref: https://pdos.csail.mit.edu/6.828/2018/readings/ia32/ioapic.pdf
// Useful: http://www.osdever.net/tutorials/view/advanced-programming-interrupt-controller

class IoApic : public IInterruptController
{
public:
    explicit IoApic(void* address);

    // Initialize the interrupt controller
    mtl::expected<void, ErrorCode> Initialize() override;

    // Acknowledge an interrupt (End of interrupt / EOI)
    void Acknowledge(int interrupt) override;

    // Enable the specified interrupt
    void Enable(int interrupt) override;

    // Disable the specified interrupt
    void Disable(int interrupt) override;

    // Return the CPU interrupt vector to use for the specified IRQ
    int MapIrqToInterrupt(int irq) const { return irq + kIrqOffset; }

private:
    static constexpr int kIrqOffset = 32;

    enum class Register : uint32_t
    {
        IOAPICID = 0x00,  // RW - ID
        IOAPICVER = 0x01, // RO - Version
        IOAPICARB = 0x02, // RO - Arbitration ID
        IOREDTBL = 0x10,  // RW - 0x10..0x3F: Redirection table (23 entries of 64 bits)
    };

    constexpr Register GetRegister(int interrupt) { return (Register)((int)Register::IOREDTBL + interrupt * 2); }

    uint32_t Read32(Register reg)
    {
        *m_ioregsel = reg;
        return *m_iowin;
    }

    void Write32(Register reg, uint32_t value)
    {
        *m_ioregsel = reg;
        *m_iowin = value;
    }

    uint64_t Read64(Register reg)
    {
        *m_ioregsel = reg;
        const uint32_t lo = *m_iowin;
        *m_ioregsel = (Register)((int)reg + 1);
        const uint64_t hi = *m_iowin;
        return (hi << 32) | lo;
    }

    void Write64(Register reg, uint64_t value)
    {
        *m_ioregsel = reg;
        *m_iowin = value;
        *m_ioregsel = (Register)((int)reg + 1);
        *m_iowin = value >> 32;
    }

    volatile Register* const m_ioregsel; // I/O register select register
    volatile uint32_t* const m_iowin;    // I/O window register
    const int m_id;                      // APIC id
    const int m_version;                 // APIC version
    const int m_interruptCount;          // Number of interrupts
    const int m_arbitrationId;           // Arbitration id
};
