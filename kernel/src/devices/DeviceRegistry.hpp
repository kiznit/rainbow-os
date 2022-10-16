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
#include "Device.hpp"
#include "DeviceInfo.hpp"
#include <memory>

struct PciDeviceRegistryEntry
{
    using Factory = Device* (*)(std::shared_ptr<PciDeviceInfo> deviceInfo);

    uint16_t vendorId;
    uint16_t deviceId;
    const Factory factory;
};

#define DECLARE_PCI_DEVICE(T) extern Device* T##Factory(std::shared_ptr<PciDeviceInfo>);

#define DEFINE_PCI_DEVICE_FACTORY(T)                                                                                               \
    Device* T##Factory(std::shared_ptr<PciDeviceInfo> deviceInfo) { return new T(std::move(deviceInfo)); }

DECLARE_PCI_DEVICE(VirtioGpu)

static PciDeviceRegistryEntry g_pciDeviceRegistry[] = {{0x1af4, 0x1050, VirtioGpuFactory}};
