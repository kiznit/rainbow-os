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
#include <rainbow/acpi.hpp>

class GicCpuInterface
{
public:
    static mtl::expected<mtl::unique_ptr<GicCpuInterface>, ErrorCode> Create(const AcpiMadt::GicCpuInterface& info);

    // Initialize the interrupt controller
    mtl::expected<void, ErrorCode> Initialize();

    // Read the Intgerrupt Acknowledge Register (IAR)
    uint32_t ReadIAR() const { return m_registers->IAR; }

    void EndOfInterrupt(int interrupt) { m_registers->EOIR = interrupt; }

private:
    struct Registers
    {
        uint32_t CTLR;
        uint32_t PMR;
        uint32_t BPR;
        uint32_t IAR;

        uint32_t EOIR;
        uint32_t RPR;
        uint32_t HPPIR;
        uint32_t ABPR;

        uint32_t AIAR;
        uint32_t AEOIR;
        uint32_t AHPPIR;

        uint32_t reserved0[41];

        uint32_t APR[4];
        uint32_t NSAPR[4];
        uint32_t reserved2[3];
        uint32_t IIDR;

        uint32_t padding[960];

        uint32_t DIR;
    };

    static_assert(sizeof(Registers) == 0x1004);

    GicCpuInterface(Registers* address);

    volatile Registers* const m_registers;
};
