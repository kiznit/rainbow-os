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
#include <cassert>
#include <metal/log.hpp>

#if defined(__x86_64__)
#include <metal/arch/x86_64/io.hpp>
#endif

namespace pci
{
#if defined(__x86_64__)
    class PcConfigSpace : public ConfigSpace
    {
    public:
        PcConfigSpace(uint16_t addressPort = 0xCF8, uint16_t dataPort = 0xCFC) : m_addressPort(addressPort), m_dataPort(dataPort) {}

        uint32_t ReadRegister(int bus, int slot, int function, int offset) const override
        {
            assert(bus >= 0 && bus <= 255);
            assert(slot >= 0 && slot <= 31);
            assert(function >= 0 && function <= 7);
            assert(offset >= 0 && offset <= 255);
            assert(!(offset & 3));

            const uint32_t address = (1u << 31) | (bus << 16) | (slot << 11) | (function << 8) | offset;
            mtl::io_out_32(m_addressPort, address);
            return mtl::io_in_32(m_dataPort);
        }

    private:
        const uint16_t m_addressPort;
        const uint16_t m_dataPort;
    };
#endif

    class PciExpressConfigSpace : public ConfigSpace
    {
    public:
        PciExpressConfigSpace(uint64_t address, int startBus, int endBus)
            : m_address(address), m_startBus(startBus), m_endBus(endBus)
        {}

        uint32_t ReadRegister(int bus, int slot, int function, int offset) const override
        {
            assert(bus >= 0 && bus <= 255);
            assert(slot >= 0 && slot <= 31);
            assert(function >= 0 && function <= 7);
            assert(offset >= 0 && offset <= 4095);
            assert(!(offset & 3));

            if (bus < m_startBus || bus > m_endBus)
                return -1;

            const uint64_t address = m_address + (bus * 256 + slot * 8 + function) * 4096 + offset;
            return *(uint32_t volatile*)address;
        }

    private:
        const uint64_t m_address;
        const int m_startBus;
        const int m_endBus;
    };

    static void EnumerateDevices(const ConfigSpace& config)
    {
        for (int bus = 0; bus != 256; ++bus)
        {
            for (int slot = 0; slot != 32; ++slot)
            {
                for (int function = 0; function != 8; ++function)
                {
                    const uint32_t reg0 = config.ReadRegister(bus, slot, function, 0);
                    const auto vendorId = reg0 & 0xFFFF;
                    if (vendorId == 0xFFFF)
                        continue;
                    const auto deviceId = reg0 >> 16;

                    MTL_LOG(Info) << "    " << bus << "/" << slot << "/" << function << ": vendor id " << mtl::hex(vendorId)
                                  << ", device id " << mtl::hex(deviceId);
                }
            }
        }
    }

#if defined(__x86_64__)
    void EnumerateDevices()
    {
        MTL_LOG(Info) << "PCI PC enumeration:";

        PcConfigSpace config;
        EnumerateDevices(config);
    }
#endif

    void EnumerateDevices(const acpi::Mcfg& mcfg)
    {
        for (const auto entry : mcfg)
        {
            MTL_LOG(Info) << "PCI Express enumeration (segment group " << entry.segment << "):";

            PciExpressConfigSpace config(entry.address, entry.startBus, entry.endBus);
            EnumerateDevices(config);
        }
    }

} // namespace pci