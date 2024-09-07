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
#include "interfaces/IClock.hpp"
#include <cstdint>
#include <metal/expected.hpp>

/*
    IA-PC HPET (High Precision Event Timers) Specification v1.0a:
    https://www.intel.com/content/dam/www/public/us/en/documents/technical-specifications/software-developers-hpet-spec-1-0a.pdf
*/

struct AcpiHpet;

// TODO: handle 32 bits vs 64 bits main counter
// TODO: handle wrap around, need interrupt handler to properly handle this
// TODO: GetTimeNs() needs to return nanoseconds, not the raw counter
// TODO: it is possible to expose the timer to user space... do we want to do that?
// TODO: use RDTSCP for clock and HPET for timers (deadline mode)
class Hpet : public IClock
{
public:
    static mtl::expected<mtl::unique_ptr<Hpet>, ErrorCode> Create();

    uint64_t GetTimeNs() const override;

    // Timers
    int GetTimerCount() const { return ((m_registers->capabilities >> 8) & 0x1F) + 1; }

    // PCI info
    uint8_t GetRevisionId() const { return m_registers->capabilities & 0xFF; }
    uint16_t GetVendorId() const { return (m_registers->capabilities >> 16) & 0xFFFF; }

    // Is the main counter 32 or 64 bits?
    bool IsCounter32Bits() const { return !IsCounter64Bits(); }
    bool IsCounter64Bits() const { return m_registers->capabilities & (1 << 13); }

private:
    struct Registers
    {
        uint64_t capabilities;          // General Capabilities and ID
        uint64_t reserved1;             // Reserved
        uint64_t configuration;         // General Configuration
        uint64_t reserved2;             // Reserved
        uint64_t irqStatus;             // General IRQ status;
        uint8_t reserved3[0xF0 - 0x28]; // Reserved
        uint64_t counter;               // Main counter (32 or 64 bits)
        uint64_t reserved4;             // Reserved
    };

    static_assert(sizeof(Registers) == 0x100);

    Hpet(const AcpiHpet& table, Registers* registers);

    volatile Registers* const m_registers;
};
