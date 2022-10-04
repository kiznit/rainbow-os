namespace pci
{
#if __x86_64__
#include <metal/arch.hpp>

    class PcConfigSpace : public PciConfigSpace
    {
    public:
        PcConfigSpace(uint16_t addressPort = 0xCF8, uint16_t dataPort = 0xCFC) : m_addressPort(addressPort), m_dataPort(dataPort) {}

        uint8_t ReadByte(int bus, int slot, int function, int offset) const override
        {
            assert(bus >= 0 && bus <= 255);
            assert(slot >= 0 && slot <= 31);
            assert(function >= 0 && function <= 7);
            assert(offset >= 0 && offset <= 255);

            const uint32_t address = (1u << 31) | (bus << 16) | (slot << 11) | (function << 8) | offset;
            mtl::x86_outl(m_addressPort, address);
            return mtl::x86_inb(m_dataPort);
        }

        uint32_t ReadRegister(int bus, int slot, int function, int offset) const override
        {
            assert(bus >= 0 && bus <= 255);
            assert(slot >= 0 && slot <= 31);
            assert(function >= 0 && function <= 7);
            assert(offset >= 0 && offset <= 255);
            assert(!(offset & 3));

            const uint32_t address = (1u << 31) | (bus << 16) | (slot << 11) | (function << 8) | offset;
            mtl::x86_outl(m_addressPort, address);
            return mtl::x86_inl(m_dataPort);
        }

    private:
        const uint16_t m_addressPort;
        const uint16_t m_dataPort;
    };

#endif

    namespace
    {
        void EnumerateDevices(const ConfigSpace& config)
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
    } // namespace

} // namespace pci