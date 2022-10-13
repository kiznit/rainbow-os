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

#pragma once
#include <metal/log.hpp>

class DeviceInfo
{
public:
    enum class AddressSpace
    {
        Pci
    };

    virtual void Write(mtl::LogStream& stream) const = 0;

protected:
    DeviceInfo(AddressSpace addressSpace) : m_addressSpace(addressSpace) {}

    // private:
    const AddressSpace m_addressSpace;
};

class PciDeviceInfo : public DeviceInfo
{
public:
    PciDeviceInfo(int segment, int bus, int slot, int function, uint16_t vendorId, uint16_t deviceId)
        : DeviceInfo(AddressSpace::Pci), m_segment(segment), m_bus(bus), m_slot(slot), m_function(function), m_vendorId(vendorId),
          m_deviceId(deviceId)
    {
    }

    void Write(mtl::LogStream& stream) const override;

private:
    const int m_segment;
    const int m_bus;
    const int m_slot;
    const int m_function;
    const uint16_t m_vendorId;
    const uint16_t m_deviceId;
};

inline mtl::LogStream& operator<<(mtl::LogStream& stream, const DeviceInfo& DeviceInfo)
{
    DeviceInfo.Write(stream);
    return stream;
}