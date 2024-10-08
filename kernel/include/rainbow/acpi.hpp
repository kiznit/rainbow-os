/*
    Copyright (c) 2024, Thierry Tremblay
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
#include <metal/log.hpp>
#include <metal/string_view.hpp>
#include <numeric>

#if defined(__clang__)
#pragma clang diagnostic ignored "-Waddress-of-packed-member"
#endif

#pragma GCC diagnostic ignored "-Waddress-of-packed-member"

// ACPI 1.0 Root System Description Pointer (RSDP)
struct AcpiRsdp
{
    char signature[8];
    uint8_t checksum;
    char oemId[6];
    uint8_t revision;
    uint32_t rsdtAddress;

    mtl::string_view GetSignature() const { return mtl::string_view(signature, 8); }
    bool VerifyChecksum() const { return (uint8_t)std::accumulate((uint8_t*)this, (uint8_t*)(this + 1), 0) == 0; }

} __attribute__((packed));

static_assert(sizeof(AcpiRsdp) == 20);

// ACPI 2.0 Root System Descriptor Pointer (RSDP)
struct AcpiRsdpExtended : AcpiRsdp
{
    uint32_t length;
    uint64_t xsdtAddress;
    uint8_t extendedChecksum;
    uint8_t reserved[3];

    bool VerifyExtendedChecksum() const { return (uint8_t)std::accumulate((uint8_t*)this, (uint8_t*)(this + 1), 0) == 0; }

} __attribute__((packed));

static_assert(sizeof(AcpiRsdpExtended) == 36);

// 5.2.3.2 Generic Address Structure (GAS)
struct AcpiAddress
{
    enum class AddressSpace : uint8_t
    {
        SystemMemory = 0,
        SystemIo = 1,
        PciConfigurationSpace = 2,
        EmbeddedController = 3,
        SmBus = 4,
        SystemCmos = 5,
        PciBarTarget = 6,
        Ipmi = 7,
        GeneralPurposeIo = 8,
        GenericSerialBus = 9,
        PlatformCommunicationsChannel = 10,
        FunctionalFixedHardware = 0x7f
    };

    AddressSpace addressSpace;
    uint8_t registerBitWidth; // Size of register in bits
    uint8_t registerBitShift; // Offset of register in bits
    uint8_t accessSize;       // 0 - undefined (legacy), 1 - uint8_t, 2 - uint16_t, 3 - uint32_t, 4 - uint64_t
    uint64_t address;

} __attribute__((packed));

static_assert(sizeof(AcpiAddress) == 12);

inline mtl::LogStream& operator<<(mtl::LogStream& stream, const AcpiAddress& address)
{
    stream << "AcpiAddress(" << (int)address.addressSpace << '/' << address.registerBitWidth << '/' << address.registerBitShift
           << '/' << address.accessSize << '/' << mtl::hex(address.address) << ')';
    return stream;
}

// 5.2.6 System Description Table Header
struct AcpiTable
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

    mtl::string_view GetSignature() const { return mtl::string_view(signature, 4); }
    bool VerifyChecksum() const { return (uint8_t)std::accumulate((uint8_t*)this, (uint8_t*)this + length, 0) == 0; }

} __attribute__((packed));

static_assert(sizeof(AcpiTable) == 36);

// 5.2.7 Root System Description Table (RSDT)
struct AcpiRsdt : AcpiTable
{
    uint32_t tables[0];

    constexpr const uint32_t* begin() const { return tables; }
    constexpr const uint32_t* end() const { return tables + size(); }
    constexpr size_t size() const { return (length - sizeof(AcpiTable)) / sizeof(tables[0]); }

} __attribute__((packed));

static_assert(sizeof(AcpiRsdt) == 36);

// 5.2.8 Extended System Description Table (XSDT)
struct AcpiXsdt : AcpiTable
{
    uint64_t tables[0];

    constexpr const uint64_t* begin() const { return tables; }
    constexpr const uint64_t* end() const { return tables + size(); }
    constexpr size_t size() const { return (length - sizeof(AcpiTable)) / sizeof(tables[0]); }

} __attribute__((packed));

static_assert(sizeof(AcpiXsdt) == 36);

// 5.2.9 Fixed ACPI Description Table (FADT)
struct AcpiFadt : AcpiTable
{
    enum class Flags : uint32_t
    {
        TmrValExt = 1 << 8
    };

    uint32_t FIRMWARE_CTRL; // Location of the FACS
    uint32_t DSDT;          // Location of the DSDT
    uint8_t reserved0;
    uint8_t preferredPMModel;
    uint16_t SCI_INT;
    uint8_t todo0[76 - 48];
    uint32_t PM_TMR_BLK; // Power Management Timer address
    uint8_t todo1[91 - 80];
    uint8_t PM_TMR_LEN; // Length of PM_TMR_BLK or 0 if not supported
    uint8_t todo2[112 - 92];
    Flags flags;

    uint8_t todo3[132 - 116];
    uint64_t X_FIRMWARE_CTRL;
    uint64_t X_DSDT;
    uint8_t todo4[208 - 148];

    // uint8_t todo3[208 - 116];

    AcpiAddress X_PM_TMR_BLK;
    uint8_t todo5[276 - 220];
} __attribute__((packed));

static_assert(sizeof(AcpiFadt) == 276);

// 5.2.12 - Multiple APIC Description Table (MADT)
struct AcpiMadt : AcpiTable
{
    enum class EntryType : uint8_t
    {
        Apic = 0,
        IoApic = 1,
        InterruptOverride = 2,
        NmiSource = 3,
        Nmi = 4,
        ApicAddressOverride = 5,
        x2Apic = 9,
        x2ApicNmi = 10,
        GicCpuInterface = 11, // GICC
        GicDistributor = 12,  // GICD
        GicMsiFrame = 13,
        GicRegistributor = 14, // GICR
        GicInterruptTranslationService = 15
    };

    struct Entry
    {
        EntryType type;
        uint8_t length;
    } __attribute__((packed));

    // 5.2.12.2 - Processor APIC Structure
    struct Apic : Entry
    {
        enum class Flags : uint32_t
        {
            Enabled = 1 << 0,
            OnlineCapable = 1 << 1,
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
        enum class Bus : uint8_t
        {
            ISA = 0
        };

        Bus bus;
        uint8_t source;
        uint32_t interrupt;
        uint16_t flags;
    } __attribute__((packed));

    // 5.2.12.7 - APIC NMI Structure
    struct Nmi : Entry
    {
        uint8_t processorId;
        uint16_t flags;
        uint8_t lint;
    } __attribute__((packed));

    // 5.2.12.8 - APIC Address Override Structure
    struct ApicAddressOverride : Entry
    {
        uint16_t reserved;
        uint64_t address;
    } __attribute__((packed));

    // 5.2.12.14 GIC CPU Interface (GICC) Structure
    struct GicCpuInterface : Entry
    {
        enum class Flags : uint32_t
        {
            Enabled = 1 << 0,
            PerformanceInterruptMode = 1 << 1,
            VgicMaintenanceInterruptMode = 1 << 2,
        };

        uint16_t reserved1;
        uint32_t id;
        uint32_t processorUid;
        Flags flags;
        uint32_t parkingProtocolVersion;
        uint32_t performanceInterruptGsiv;
        uint64_t parkedAddress;
        uint64_t address;
        uint64_t gicvAddress;
        uint64_t gichAddress;
        uint32_t vgicMaintenanceInterrupt;
        uint64_t gicrAddress;
        uint64_t mpidr;
        uint8_t processorPowerEfficiencyClass;
        uint8_t reserved2;
        uint16_t speOVerflowInterrupt;
    } __attribute__((packed));
    static_assert(sizeof(GicCpuInterface) == 80);

    // 5.2.12.15 GIC Distributor (GICD) Structure
    struct GicDistributor : Entry
    {
        uint16_t reserved1;
        uint32_t id;
        uint64_t address;
        uint32_t systemVectorTable;
        uint8_t version;
        uint8_t reserved[3];
    } __attribute__((packed));
    static_assert(sizeof(GicDistributor) == 24);

    // 5.2.12.16 GIC MSI Frame Structure
    struct GicMsiFrame : Entry
    {
        enum class Flags : uint32_t
        {
            SpiCountBaseSelect = 1 << 0,
        };

        uint16_t reserved1;
        uint32_t id;
        uint64_t address;
        Flags flags;
        uint16_t spiCount;
        uint16_t spiBase;

    } __attribute__((packed));
    static_assert(sizeof(GicMsiFrame) == 24);

    // 5.2.12.17 GIC Redistributor (GICR) Structure
    struct GicRedistributor : Entry
    {
        uint16_t reserved1;
        uint64_t address;
        uint32_t length;
    } __attribute__((packed));
    static_assert(sizeof(GicRedistributor) == 16);

    // 5.2.12.18 GIC Interrupt Translation Service (ITS) Structure
    struct GicInterruptTranslation : Entry
    {
        uint16_t reserved1;
        uint32_t id;
        uint64_t address;
        uint32_t reserved2;
    } __attribute__((packed));
    static_assert(sizeof(GicInterruptTranslation) == 20);

    enum Flag
    {
        PcatCompat = 1 // Dual 8259 available
    };

    uint32_t apicAddress;
    uint32_t flags;
    Entry entries[0];
} __attribute__((packed));

static_assert(sizeof(AcpiMadt) == 44);

// PCI Express memory mapped configuration space (MCFG)
struct AcpiMcfg : AcpiTable
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
    constexpr size_t size() const { return (length - sizeof(AcpiTable)) / sizeof(configs[0]); }

} __attribute__((packed));

static_assert(sizeof(AcpiMcfg) == 44);
static_assert(sizeof(AcpiMcfg::Config) == 16);

// HPET Description Table (HPET)
struct AcpiHpet : AcpiTable
{
    uint32_t eventTimerBlockId; // Hardware ID of Event Timer Block
    AcpiAddress address;        // Base address of the Event Timer Block
    uint8_t hpetNumber;         // HPET sequence number.
    uint16_t minClockTick;      // Minimum clock ticks
    uint8_t attributes;         // Page protection and OEM attribute

} __attribute__((packed));

static_assert(sizeof(AcpiHpet) == 56);

// Generic Timer Description Table (ARM)
struct AcpiGenericTimer : AcpiTable
{
    enum class Flags : uint32_t
    {
        InterruptMode = 1 << 0,     // 0 - level triggered, 1 - edge triggered
        InterruptPolatiry = 1 << 1, // 0 - active high, 1 - active low
        AlwaysOnCapability = 1 << 2,
    };

    enum class TimerType : uint8_t
    {
        Standard = 0,
        SbsaGenericWatchdog = 1,
    };

    struct Timer
    {
        TimerType type;
        uint16_t length;
        uint8_t reserved;
    };

    uint64_t counterControlAddress;
    uint32_t reserved;
    uint32_t secureEL1TimerGsiv;
    Flags secureEL1TimerFlags;
    uint32_t nonSecureEL1TimerGsiv;
    Flags nonSecureEL1TimerFlags;
    uint32_t virtualEL1TimerGsiv;
    Flags virtualEL1TimerFlags;
    uint32_t EL2TimerGsiv;
    Flags EL2TimerFlags;
    uint64_t counterReadAddress;
    uint32_t timerCount;
    uint32_t timerOffset;
    uint32_t virtualEL2TimerGsiv;
    Flags virtualEL2TimerFlags;
    Timer timers[];
} __attribute__((packed));

static_assert(sizeof(AcpiGenericTimer) == 104);
