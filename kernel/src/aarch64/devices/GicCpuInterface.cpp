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

#include "GicCpuInterface.hpp"
#include "arch.hpp"

std::expected<std::unique_ptr<GicCpuInterface>, ErrorCode> GicCpuInterface::Create(const AcpiMadt::GicCpuInterface& info)
{
    auto pageCount = mtl::AlignUp(sizeof(Registers), mtl::kMemoryPageSize) >> mtl::kMemoryPageShift;
    auto registers = ArchMapSystemMemory(info.address, pageCount, mtl::PageFlags::MMIO);
    if (!registers)
        return std::unexpected(registers.error());

    auto gic = std::unique_ptr(new GicCpuInterface(static_cast<Registers*>(*registers)));
    if (!gic)
        return std::unexpected(ErrorCode::OutOfMemory);

    auto result = gic->Initialize();
    if (!result)
        return std::unexpected(result.error());

    return gic;
}

GicCpuInterface::GicCpuInterface(Registers* registers) : m_registers(registers)
{
}

std::expected<void, ErrorCode> GicCpuInterface::Initialize()
{
    m_registers->CTLR = 1;   // Enable
    m_registers->PMR = 0xff; // Priority masking

    MTL_LOG(Info) << "[GIC] GIC CPU Interface initialized at " << m_registers;
    return {};
}
