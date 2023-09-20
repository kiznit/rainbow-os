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

#pragma once

#include <cstdint>

class Acpi;

struct PciConfigSpace
{
    uint16_t vendorId;
    uint16_t deviceId;
    uint16_t command;
    uint16_t status;
    uint8_t revisionId;
    uint8_t progInterface;
    uint8_t subClass;
    uint8_t baseClass;
    uint8_t cacheLineSize;
    uint8_t latencyTimer;
    uint8_t headerType;
    uint8_t BIST;
} __attribute__((packed));

static_assert(sizeof(PciConfigSpace) == 0x10);

struct PciConfigSpaceType0 : PciConfigSpace
{
    uint32_t bar[6];
    uint32_t cardsbusCisPointer;
    uint16_t subsystemVendorId;
    uint16_t subsystemId;
    uint32_t expensionRomBaseAddress;
    uint8_t capabilitiesPointer;
    uint8_t reserved[7];
    uint8_t interruptLine;
    uint8_t interruptPin;
    uint8_t minGrant;
    uint8_t maxLatency;
} __attribute__((packed));

static_assert(sizeof(PciConfigSpaceType0) == 0x40);

// TODO: return error codes where appropriate

void PciInitialize(const Acpi* acpi);

// Get a pointer to the specified device's configuration space
volatile PciConfigSpace* PciMapConfigSpace(int segment, int bus, int slot, int function);

uint8_t PciRead8(int segment, int bus, int slot, int function, int offset);
uint16_t PciRead16(int segment, int bus, int slot, int function, int offset);
uint32_t PciRead32(int segment, int bus, int slot, int function, int offset);
void PciWrite8(int segment, int bus, int slot, int function, int offset, uint8_t value);
void PciWrite16(int segment, int bus, int slot, int function, int offset, uint16_t value);
void PciWrite32(int segment, int bus, int slot, int function, int offset, uint32_t value);
