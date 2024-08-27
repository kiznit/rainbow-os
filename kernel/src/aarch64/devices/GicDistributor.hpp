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

#include "interfaces/IInterruptController.hpp"
#include <cstdint>
#include <rainbow/acpi.hpp>

class GicDistributor : public IInterruptController
{
public:
    enum class Trigger
    {
        Level = 0, // Asserted when the signal level is active and deasserted when the level is not active
        Edge = 1,  // Asserted on rising edge of signal, remains asserted until cleared
    };

    static mtl::expected<std::unique_ptr<GicDistributor>, ErrorCode> Create(const AcpiMadt::GicDistributor& info);

    // Initialize the interrupt controller
    mtl::expected<void, ErrorCode> Initialize() override;

    // Interrupt configuration
    void SetGroup(int interrupt, int group);
    void SetPriority(int interrupt, uint8_t priority);
    void SetTargetCpu(int interrupt, uint8_t cpuMask);
    void SetTrigger(int interrupt, Trigger trigger);

    // Acknowledge an interrupt (End of interrupt / EOI)
    void Acknowledge(int interrupt) override;

    // Enable the specified interrupt
    void Enable(int interrupt) override;

    // Disable the specified interrupt
    void Disable(int interrupt) override;

private:
    struct Registers
    {
        uint32_t CTLR;
        uint32_t TYPER;
        uint32_t IIDR;
        uint32_t reserved0;
        uint32_t STATUSR;
        uint32_t reserved1[11];
        uint32_t SETSPI_NSR;
        uint32_t reserved2;
        uint32_t CLRSPI_NSR;
        uint32_t reserved3;
        uint32_t SETSPI_SR;
        uint32_t reserved4;
        uint32_t CLRSPI_SR;
        uint32_t reserved5[9];
        uint32_t IGROUPR[32];
        uint32_t ISENABLER[32];
        uint32_t ICENABLER[32];
        uint32_t ISPENDR[32];
        uint32_t ICPENDR[32];
        uint32_t ISACTIVER[32];
        uint32_t ICACTIVER[32];
        uint32_t IPRIORITYR[255];
        uint32_t reserved6;
        uint32_t ITARGETSR[255];
        uint32_t reserved7;
        uint32_t ICFGR[64];
        uint32_t IGRPMODR[32];
        uint32_t reserved8[32];
        uint32_t NSACR[64];
        uint32_t SGIR;
        uint32_t reserved9[3];
        uint32_t CPENDSGIR[4];
        uint32_t SPENDSGIR[4];
    };

    static_assert(sizeof(Registers) == 0xF30);

    GicDistributor(Registers* address);

    volatile Registers* const m_registers;
};
