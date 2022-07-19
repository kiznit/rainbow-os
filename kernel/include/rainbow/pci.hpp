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

#include <cstdint>
#include <rainbow/acpi.hpp>

namespace pci
{
    // struct Header
    // {
    //     // Register 0
    //     uint16_t vendorId;
    //     uint16_t deviceId;
    //     // Register 1
    //     uint16_t command;
    //     uint16_t status;
    //     // Register 2
    //     uint8_t revisionId;
    //     uint8_t progInterface;
    //     uint8_t subclass;
    //     uint8_t classCode;
    //     // Register 3
    //     uint8_t cacheLineSize;
    //     uint8_t latencyTimer;
    //     uint8_t headerType;
    //     uint8_t bist; // build-in self-test
    //     // Registers 4-9
    //     uint32_t bars[6];
    //     // Register 10
    //     uint32_t cardbusCisPointer;
    //     // Register 11
    //     uint16_t subsystemVendorId;
    //     uint16_t subsystemDeviceId;
    //     // Register 12
    //     uint32_t romBaseAddress;
    //     // Registers 13-14
    //     uint32_t reserved[2];
    //     // Register 15
    //     uint8_t interruptLine;
    //     uint8_t interruptPin;
    //     uint8_t minGrant;
    //     uint8_t maxLatency;

    // } __attribute__((packed));

    // static_assert(sizeof(Header) == 64);

    struct ConfigSpace
    {
        virtual uint32_t ReadRegister(int bus, int slot, int function, int offset) const = 0;
    };

    void EnumerateDevices();
    void EnumerateDevices(const acpi::Mcfg& mcfg);

} // namespace pci
