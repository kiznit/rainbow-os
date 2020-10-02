/*
    Copyright (c) 2020, Thierry Tremblay
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

#ifndef _RAINBOW_ACPI_HPP
#define _RAINBOW_ACPI_HPP

#include <stdint.h>


namespace Acpi
{
    // ACPI 1.0 Root System Description Pointer (RSDP)
    struct Rsdp
    {
        char signature[8];
        uint8_t checksum;
        char oemId[6];
        uint8_t revision;
        uint32_t rsdtAddress;
    } __attribute__ ((packed));

    static_assert(sizeof(Rsdp) == 20);


    // ACPI 2.0 Root System Descriptor Pointer (RSDP)
    struct Rsdp20 : Rsdp
    {
        uint32_t length;
        uint64_t xsdtAddress;
        uint8_t extendedChecksum;
        uint8_t reserved[3];
    } __attribute__ ((packed));

    static_assert(sizeof(Rsdp20) == 36);


    // 5.2.6 System Description Table Header
    struct Table
    {
        uint32_t signature;
        uint32_t length;
        uint8_t  revision;
        uint8_t  checksum;
        uint8_t  oemId[6];
        uint8_t  oemTableId[8];
        uint32_t oemRevision;
        uint32_t creatorId;
        uint32_t creatorRevision;
    } __attribute__ ((packed));

    static_assert(sizeof(Table) == 36);


    // 5.2.7 Root System Description Table (RSDT)
    struct Rsdt : Table
    {
        uint32_t tables[0];
    } __attribute__ ((packed));

    static_assert(sizeof(Rsdt) == 36);


    // 5.2.8 Extended System Description Table (XSDT)
    struct Xsdt : Table
    {
        uint64_t tables[0];
    } __attribute__ ((packed));

    static_assert(sizeof(Xsdt) == 36);


    // 5.2.12 - Multiple APIC Description Table (MADT)
    struct Madt : Table
    {
        struct Entry
        {
            uint8_t type;
            uint8_t length;
        } __attribute__ ((packed));

        // 5.2.12.2 - Processor Local APIC Structure
        struct LocalApic : Entry
        {
            uint8_t processorId;
            uint8_t id;
            uint32_t flags;

            enum
            {
                FLAG_ENABLED        = 0x01,
                FLAG_ONLINE_CAPABLE = 0x02
            };

        } __attribute__ ((packed));

        // 5.2.12.3 - I/O APIC
        struct IoApic : Entry
        {
            uint8_t id;
            uint8_t reserved;
            uint32_t address;
            uint32_t interruptBase;
        } __attribute__ ((packed));

        // 5.2.12.5 - Interrupt Source Override Structure
        struct InterruptOverride : Entry
        {
            uint8_t bus;
            uint8_t source;
            uint32_t interrupt;
            uint16_t flags;
        } __attribute__ ((packed));

        // 5.2.12.7 - Local APIC NMI Structure
        struct Nmi : Entry
        {
            uint8_t processorId;
            uint16_t flags;
            uint8_t lint;
        } __attribute__ ((packed));

        // 5.2.12.8 - Local APIC Address Override Structure
        struct LocalApicAddressOverride : Entry
        {
            uint16_t reserved;
            uint64_t address;
        } __attribute__ ((packed));

        uint32_t localApicAddress;
        uint32_t flags;
        Entry entries[0];
    } __attribute__ ((packed));

    static_assert(sizeof(Madt) == 44);


} // namespace Acpi

#endif
