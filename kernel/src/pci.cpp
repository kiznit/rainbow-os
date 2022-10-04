/*
    Copyright (c) 2022, Thierry Tremblay
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

#include "pci.hpp"
#include "acpi/acpi.hpp"
#include "memory.hpp"
#include <algorithm>
#include <cassert>
#include <metal/log.hpp>

namespace
{
    static constexpr mtl::PhysicalAddress kPciMemoryOffset = 0xFFFF800000000000ull;

    AcpiMcfg* g_mcfg{};

    const AcpiMcfg::Config* PciFindConfig(int segment, int bus)
    {
        if (!g_mcfg) [[unlikely]]
            return nullptr;

        const auto it = std::find_if(g_mcfg->begin(), g_mcfg->end(), [=](const AcpiMcfg::Config& config) {
            return segment == config.segment && bus >= config.startBus && bus <= config.endBus;
        });

        if (it == g_mcfg->end()) [[unlikely]]
            return nullptr;

        return it;
    }

} // namespace

void PciInitialize()
{
    g_mcfg = (AcpiMcfg*)AcpiFindTable("MCFG");
    assert(g_mcfg);

    // Map PCI memory
    for (const auto& config : *g_mcfg)
    {
        const auto virtualAddress = (void*)(config.address + kPciMemoryOffset);
        const auto pageCount = (32 * 8 * 4096ull) * (config.endBus - config.startBus + 1) >> mtl::kMemoryPageShift;
        MapPages(config.address, virtualAddress, pageCount, mtl::PageFlags::MMIO);
        MTL_LOG(Info) << "Mapped PCIE memory: " << mtl::hex(config.address) << " to " << virtualAddress << ", page count "
                      << pageCount;
    }
}

uint8_t PciRead8(int segment, int bus, int slot, int function, int offset)
{
    const auto config = PciFindConfig(segment, bus);
    if (config)
    {
        if (slot < 0 || slot > 31 || function < 0 || function > 7 || offset > 4095) [[unlikely]]
            return -1;

        const uint64_t address =
            config->address + kPciMemoryOffset + ((bus - config->startBus) * 256 + slot * 8 + function) * 4096 + offset;
        return *(uint8_t volatile*)address;
    }

    return -1;
}

void PciWrite8(int segment, int bus, int slot, int function, int offset, uint8_t value)
{
    const auto config = PciFindConfig(segment, bus);
    if (config)
    {
        if (slot < 0 || slot > 31 || function < 0 || function > 7 || offset > 4095) [[unlikely]]
            return;

        const uint64_t address =
            config->address + kPciMemoryOffset + ((bus - config->startBus) * 256 + slot * 8 + function) * 4096 + offset;
        *(uint8_t volatile*)address = value;
    }
}

uint16_t PciRead16(int segment, int bus, int slot, int function, int offset)
{
    const auto config = PciFindConfig(segment, bus);
    if (config)
    {
        if (slot < 0 || slot > 31 || function < 0 || function > 7 || offset > 4094) [[unlikely]]
            return -1;

        const uint64_t address =
            config->address + kPciMemoryOffset + ((bus - config->startBus) * 256 + slot * 8 + function) * 4096 + offset;
        return *(uint16_t volatile*)address;
    }

    return -1;
}

void PciWrite16(int segment, int bus, int slot, int function, int offset, uint16_t value)
{
    const auto config = PciFindConfig(segment, bus);
    if (config)
    {
        if (slot < 0 || slot > 31 || function < 0 || function > 7 || offset > 4094) [[unlikely]]
            return;

        const uint64_t address =
            config->address + kPciMemoryOffset + ((bus - config->startBus) * 256 + slot * 8 + function) * 4096 + offset;
        *(uint16_t volatile*)address = value;
    }
}

uint32_t PciRead32(int segment, int bus, int slot, int function, int offset)
{
    const auto config = PciFindConfig(segment, bus);
    if (config)
    {
        if (slot < 0 || slot > 31 || function < 0 || function > 7 || offset > 4092) [[unlikely]]
            return -1;

        const uint64_t address =
            config->address + kPciMemoryOffset + ((bus - config->startBus) * 256 + slot * 8 + function) * 4096 + offset;
        return *(uint32_t volatile*)address;
    }

    return -1;
}

void PciWrite32(int segment, int bus, int slot, int function, int offset, uint32_t value)
{
    const auto config = PciFindConfig(segment, bus);
    if (config)
    {
        if (slot < 0 || slot > 31 || function < 0 || function > 7 || offset > 4092) [[unlikely]]
            return;

        const uint64_t address =
            config->address + kPciMemoryOffset + ((bus - config->startBus) * 256 + slot * 8 + function) * 4096 + offset;
        *(uint32_t volatile*)address = value;
    }
}
