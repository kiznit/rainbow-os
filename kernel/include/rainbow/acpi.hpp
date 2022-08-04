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

#include <cstddef>
#include <cstdint>
#include <numeric>
#include <string_view>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waddress-of-packed-member"

namespace acpi
{
    // ACPI 1.0 Root System Description Pointer (RSDP)
    struct Rsdp
    {
        char signature[8];
        uint8_t checksum;
        char oemId[6];
        uint8_t revision;
        uint32_t rsdt;

        std::string_view GetSignature() const { return std::string_view(signature, 8); }
        bool VerifyChecksum() const { return (uint8_t)std::accumulate((uint8_t*)this, (uint8_t*)(this + 1), 0) == 0; }

    } __attribute__((packed));

    static_assert(sizeof(Rsdp) == 20);

    // ACPI 2 Root System Descriptor Pointer (RSDP)
    struct RsdpExtended : Rsdp
    {
        uint32_t length;
        uint64_t xsdt;
        uint8_t extendedChecksum;
        uint8_t reserved[3];

        bool VerifyExtendedChecksum() const { return (uint8_t)std::accumulate((uint8_t*)this, (uint8_t*)(this + 1), 0) == 0; }

    } __attribute__((packed));

    static_assert(sizeof(RsdpExtended) == 36);

    // 5.2.3.2 Generic Address Structure (GAS)
    struct GenericAddress
    {
        enum class Space : uint8_t
        {
            SystemMemory = 0,
            SystemIO = 1,
        };

        Space addressSpaceId; // 0 - system memory, 1 - system I/O, ...
        uint8_t registerBitWidth;
        uint8_t registerBitShift;
        uint8_t reserved;
        uint64_t address;

    } __attribute__((packed));

    static_assert(sizeof(GenericAddress) == 12);

    // 5.2.6 System Description Table Header
    struct Table
    {
        char signature[4];
        uint32_t length;
        uint8_t revision;
        uint8_t checksum;
        uint8_t oemId[6];
        uint8_t oemTableId[8];
        uint32_t oemRevision;
        uint32_t creatorId;
        uint32_t creatorRevision;

        std::string_view GetSignature() const { return std::string_view(signature, 4); }
        bool VerifyChecksum() const { return (uint8_t)std::accumulate((uint8_t*)this, (uint8_t*)this + length, 0) == 0; }

    } __attribute__((packed));

    static_assert(sizeof(Table) == 36);

    // 5.2.7 Root System Description Table (RSDT)
    struct Rsdt : Table
    {
        uint32_t tables[0];

        constexpr const uint32_t* begin() const { return tables; }
        constexpr const uint32_t* end() const { return tables + size(); }
        constexpr size_t size() const { return (length - sizeof(Table)) / sizeof(tables[0]); }

    } __attribute__((packed));

    static_assert(sizeof(Rsdt) == 36);

    // 5.2.8 Extended System Description Table (XSDT)
    struct Xsdt : Table
    {
        uint64_t tables[0];

        constexpr const uint64_t* begin() const { return tables; }
        constexpr const uint64_t* end() const { return tables + size(); }
        constexpr size_t size() const { return (length - sizeof(Table)) / sizeof(tables[0]); }

    } __attribute__((packed));

    static_assert(sizeof(Xsdt) == 36);

    // 5.2.9 Fixed ACPI Description Table (FADT)
    struct Fadt : Table
    {
        enum class Flags : uint32_t
        {
            TmrValExt = 1 << 8
        };

        uint8_t todo0[76 - 36];
        uint32_t PM_TMR_BLK; // Power Management Timer address
        uint8_t todo1[91 - 80];
        uint8_t PM_TMR_LEN; // Length of PM_TMR_BLK or 0 if not supported
        uint8_t todo2[112 - 92];
        Flags flags;
        uint8_t todo3[208 - 116];
        GenericAddress X_PM_TMR_BLK;
        uint8_t todo4[276 - 220];
    };

    static_assert(sizeof(Fadt) == 276);

    // 5.2.12 - Multiple APIC Description Table (MADT)
    struct Madt : Table
    {
        struct Entry
        {
            uint8_t type;
            uint8_t length;
        } __attribute__((packed));

        // 5.2.12.2 - Processor Local APIC Structure
        struct LocalApic : Entry
        {
            enum class Flags : uint32_t
            {
                Enabled = 0x01,
                OnlineCapable = 0x02
            };

            uint8_t processorId;
            uint8_t id;
            Flags flags;

        } __attribute__((packed));

        // 5.2.12.3 - I/O APIC
        struct IoApic : Entry
        {
            uint8_t id;
            uint8_t reserved;
            uint32_t address;
            uint32_t interruptBase;
        } __attribute__((packed));

        // 5.2.12.5 - Interrupt Source Override Structure
        struct InterruptOverride : Entry
        {
            uint8_t bus;
            uint8_t source;
            uint32_t interrupt;
            uint16_t flags;
        } __attribute__((packed));

        // 5.2.12.7 - Local APIC NMI Structure
        struct Nmi : Entry
        {
            uint8_t processorId;
            uint16_t flags;
            uint8_t lint;
        } __attribute__((packed));

        // 5.2.12.8 - Local APIC Address Override Structure
        struct LocalApicAddressOverride : Entry
        {
            uint16_t reserved;
            uint64_t address;
        } __attribute__((packed));

        uint32_t localApicAddress;
        uint32_t flags;
        Entry entries[0];
    } __attribute__((packed));

    static_assert(sizeof(Madt) == 44);

    // PCI Express memory mapped configuration space (MCFG)
    struct Mcfg : Table
    {
        struct Config
        {
            uint64_t address; // Base Address
            uint16_t segment; // PCI Segment Group Number
            uint8_t startBus; // Start PCI Bus Number
            uint8_t endBus;   // End PCI Bus Number
            uint8_t reserved[4];
        };

        uint8_t reserved[8];
        Config configs[0];

        constexpr const Config* begin() const { return configs; }
        constexpr const Config* end() const { return configs + size(); }
        constexpr size_t size() const { return (length - sizeof(Table)) / sizeof(configs[0]); }

    } __attribute__((packed));

    static_assert(sizeof(Mcfg) == 44);
    static_assert(sizeof(Mcfg::Config) == 16);
} // namespace acpi

#pragma GCC diagnostic pop