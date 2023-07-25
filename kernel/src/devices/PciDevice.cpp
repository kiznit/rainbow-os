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

#include "PciDevice.hpp"
#include "pci/Vga.hpp"
#include "pci/VirtioGpu.hpp"

std::shared_ptr<PciDevice> PciDevice::Create(volatile PciConfigSpace* configSpace)
{
    // Check specific vendor / device id pairs
    if (configSpace->vendorId == 0x1af4 && configSpace->deviceId == 0x1050)
        return std::make_shared<VirtioGpu>(configSpace);

    // Check class codes
    // if (configSpace->baseClass == 0x03 && configSpace->subClass == 0x00 && configSpace->progInterface == 0x00)
    //     return std::make_shared<Vga>(configSpace);

    return std::make_shared<PciDevice>(Class::Unknown, configSpace);
}

void PciDevice::Write(mtl::LogStream& stream) const
{
    stream << "PCI Device " << mtl::hex(m_configSpace->vendorId) << ':' << mtl::hex(m_configSpace->deviceId) << " ("
           << GetDescription() << ')';
}
