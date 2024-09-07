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

#include "GicDistributor.hpp"
#include "Cpu.hpp"
#include "arch.hpp"

mtl::expected<mtl::unique_ptr<GicDistributor>, ErrorCode> GicDistributor::Create(const AcpiMadt::GicDistributor& info)
{
    auto pageCount = mtl::AlignUp(sizeof(Registers), mtl::kMemoryPageSize) >> mtl::kMemoryPageShift;
    auto registers = ArchMapSystemMemory(info.address, pageCount, mtl::PageFlags::MMIO);
    if (!registers)
        return mtl::unexpected(registers.error());

    auto gic = mtl::unique_ptr(new GicDistributor(static_cast<Registers*>(*registers)));
    if (!gic)
        return mtl::unexpected(ErrorCode::OutOfMemory);

    auto result = gic->Initialize();
    if (!result)
        return mtl::unexpected(result.error());

    return gic;
}

GicDistributor::GicDistributor(Registers* registers) : m_registers(registers)
{
}

mtl::expected<void, ErrorCode> GicDistributor::Initialize()
{
    m_registers->CTLR = 1; // Enable

    MTL_LOG(Info) << "[GIC] GIC Distributor initialized at " << m_registers;
    return {};
}

void GicDistributor::Acknowledge(int interrupt)
{
    CpuGetGicCpuInterface()->EndOfInterrupt(interrupt);
}

void GicDistributor::Enable(int interrupt)
{
    const auto index = interrupt / 32;
    const auto mask = 1 << (interrupt % 32);
    m_registers->ISENABLER[index] = mask;
}

void GicDistributor::Disable(int interrupt)
{
    m_registers->ICENABLER[interrupt / 32] = 1 << (interrupt % 32);
}

void GicDistributor::SetGroup(int interrupt, int group)
{
    const auto index = interrupt / 32;
    const auto mask = 1 << (interrupt % 32);

    auto value = m_registers->IGROUPR[index];

    if (group)
        value |= mask;
    else
        value &= mask;

    m_registers->IGROUPR[index] = value;
}

void GicDistributor::SetPriority(int interrupt, uint8_t priority)
{
    m_registers->IPRIORITYR[interrupt] = priority;
}

void GicDistributor::SetTargetCpu(int interrupt, uint8_t cpuMask)
{
    assert(interrupt > 7);

    m_registers->ITARGETSR[interrupt] = cpuMask;
}

void GicDistributor::SetTrigger(int interrupt, Trigger trigger)
{
    const auto index = interrupt / 16;
    const auto mask = 3 << (interrupt % 16);

    auto value = m_registers->ICFGR[index] & mask;
    value |= (int)trigger << ((interrupt % 16) + 1);
    m_registers->ICFGR[index] = value;
}
