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

#include "Hpet.hpp"
#include "acpi/Acpi.hpp"
#include <metal/log.hpp>

std::expected<std::unique_ptr<Hpet>, ErrorCode> Hpet::Create()
{
    auto table = AcpiFindTable<AcpiHpet>("HPET");
    if (!table)
    {
        MTL_LOG(Fatal) << "[HPET] HPET not found";
        return std::unexpected(ErrorCode::Unsupported);
    }

    if (table->address.addressSpace != AcpiAddress::AddressSpace::SystemMemory)
    {
        MTL_LOG(Fatal) << "[HPET] HPET not in system memory";
        return std::unexpected(ErrorCode::Unsupported);
    }

    MTL_LOG(Info) << "[HPET] eventTimerBlockId: " << table->eventTimerBlockId;
    MTL_LOG(Info) << "[HPET] address: " << table->address;
    MTL_LOG(Info) << "[HPET] hpetNumber: " << table->hpetNumber;
    MTL_LOG(Info) << "[HPET] minClockTick: " << table->minClockTick;
    MTL_LOG(Info) << "[HPET] attributes: " << mtl::hex(table->attributes);

    auto registers = ArchMapSystemMemory(table->address.address, 1, mtl::PageFlags::MMIO);
    if (!registers)
        return std::unexpected(registers.error());

    auto result = std::unique_ptr(new Hpet(*table, (Registers*)registers.value()));
    if (!result)
        return std::unexpected(ErrorCode::OutOfMemory);

    return result;
}

Hpet::Hpet(const AcpiHpet& /*table*/, Registers* registers) : m_registers(registers)
{
    const auto period = registers->capabilities >> 32;
    const auto frequency = 1000000000000000ull / period;

    MTL_LOG(Info) << "[HPET] vendor id: " << mtl::hex(GetVendorId());
    MTL_LOG(Info) << "[HPET] revision id: " << mtl::hex(GetVendorId());
    MTL_LOG(Info) << "[HPET] counter width: " << (IsCounter64Bits() ? 64 : 32);
    MTL_LOG(Info) << "[HPET] period: " << (period / 1000000) << " ns";
    MTL_LOG(Info) << "[HPET] frequency: " << frequency << " Hz";
    MTL_LOG(Info) << "[HPET] timers count: " << GetTimerCount();

    // Initialize main counter
    registers->configuration = 1;

    MTL_LOG(Info) << "[HPET] HPET initialized";
}

uint64_t Hpet::GetTimeNs() const
{
    return m_registers->counter;
}
