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

#include "pci.hpp"
#include "acpi/acpi.hpp"
#include "arch.hpp"
#include "devices/DeviceManager.hpp"
#include "devices/PciDevice.hpp"
#include "memory.hpp"
#include <algorithm>
#include <cassert>
#include <metal/helpers.hpp>
#include <metal/log.hpp>

namespace
{
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

static void PciEnumerateDevices()
{
    if (!g_mcfg) [[unlikely]]
        return;

    for (const auto& mcfg : *g_mcfg)
    {
        for (int bus = mcfg.startBus; bus <= mcfg.endBus; ++bus)
        {
            for (int slot = 0; slot != 32; ++slot)
            {
                for (int function = 0; function != 8; ++function)
                {
                    if (auto configSpace = PciMapConfigSpace(mcfg.segment, bus, slot, function))
                    {
                        if (configSpace->vendorId == 0xFFFF)
                            continue;

                        const auto device = PciDevice::Create(configSpace);
                        MTL_LOG(Info) << "[PCI] (" << mtl::hex<uint16_t>(mcfg.segment) << '/' << mtl::hex<uint8_t>(bus) << '/'
                                      << mtl::hex<uint8_t>(slot) << '/' << mtl::hex<uint8_t>(function) << ") " << *device;
                        g_deviceManager.AddDevice(std::move(device));

                        // Check if we are dealing with a multi-function device or not
                        if (function == 0)
                        {
                            if (!(configSpace->headerType & 0x80))
                                break;
                        }
                    }
                }
            }
        }
    }
}

void PciInitialize()
{
    g_mcfg = (AcpiMcfg*)AcpiFindTable("MCFG");
    if (!g_mcfg)
    {
        MTL_LOG(Warning) << "[PCI] ACPI MCFG table not found, PCIE not available";
        return;
    }

    // Map PCI memory
    for (const auto& config : *g_mcfg)
    {
        const auto pageCount = (32 * 8 * 4096ull) * (config.endBus - config.startBus + 1) >> mtl::kMemoryPageShift;
        const auto virtualAddress = ArchMapSystemMemory(config.address, pageCount, mtl::PageFlags::MMIO);
        if (virtualAddress)
        {
            MTL_LOG(Info) << "[PCI] Mapped PCIE configuration space: " << mtl::hex(config.address) << " to " << *virtualAddress
                          << ", page count " << pageCount;
        }
        else
        {
            MTL_LOG(Fatal) << "[PCI] Failed to map PCIE configuration space: " << mtl::hex(config.address) << " to "
                           << *virtualAddress << ", page count " << pageCount << ": " << virtualAddress.error();
            std::abort();
        }
    }

    PciEnumerateDevices();
}

volatile PciConfigSpace* PciMapConfigSpace(int segment, int bus, int slot, int function)
{
    if (slot < 0 || slot > 31 || function < 0 || function > 7) [[unlikely]]
        return nullptr;

    const auto config = PciFindConfig(segment, bus);
    if (config)
    {
        const uint64_t address = config->address + ((bus - config->startBus) * 256 + slot * 8 + function) * 4096;
        return reinterpret_cast<volatile PciConfigSpace*>(ArchGetSystemMemory(address));
    }

    return nullptr;
}

template <typename T>
static inline T PciReadImpl(int segment, int bus, int slot, int function, int offset)
{
    if (auto address = PciMapConfigSpace(segment, bus, slot, function))
    {
        if (offset > 4096 - (int)sizeof(T)) [[unlikely]]
            return -1;

        return *reinterpret_cast<volatile T*>(mtl::AdvancePointer(address, offset));
    }

    return -1;
}

template <typename T>
static inline void PciWriteImpl(int segment, int bus, int slot, int function, int offset, T value)
{
    if (auto address = PciMapConfigSpace(segment, bus, slot, function))
    {
        if (offset > 4096 - (int)sizeof(T)) [[unlikely]]
            return;

        *reinterpret_cast<volatile T*>(mtl::AdvancePointer(address, offset)) = value;
    }
}

uint8_t PciRead8(int segment, int bus, int slot, int function, int offset)
{
    return PciReadImpl<uint8_t>(segment, bus, slot, function, offset);
}

uint16_t PciRead16(int segment, int bus, int slot, int function, int offset)
{
    return PciReadImpl<uint16_t>(segment, bus, slot, function, offset);
}

uint32_t PciRead32(int segment, int bus, int slot, int function, int offset)
{
    return PciReadImpl<uint32_t>(segment, bus, slot, function, offset);
}

void PciWrite8(int segment, int bus, int slot, int function, int offset, uint8_t value)
{
    PciWriteImpl<uint8_t>(segment, bus, slot, function, offset, value);
}

void PciWrite16(int segment, int bus, int slot, int function, int offset, uint16_t value)
{
    PciWriteImpl<uint16_t>(segment, bus, slot, function, offset, value);
}

void PciWrite32(int segment, int bus, int slot, int function, int offset, uint32_t value)
{
    PciWriteImpl<uint32_t>(segment, bus, slot, function, offset, value);
}
