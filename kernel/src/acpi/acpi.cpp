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

#include "acpi.hpp"
#include "lai.hpp"
#include "memory.hpp"
#include <lai/helpers/pm.h>
#include <lai/helpers/sci.h>
#include <metal/arch.hpp>
#include <metal/log.hpp>
#include <rainbow/boot.hpp>
#include <rainbow/uefi.hpp>

const AcpiRsdt* g_rsdt{};
const AcpiXsdt* g_xsdt{};

static void AcpiLogTable(const AcpiTable& table, mtl::PhysicalAddress address)
{
    if (table.VerifyChecksum())
        MTL_LOG(Info) << "[ACPI] Table " << table.GetSignature() << " found at " << mtl::hex(address) << ", Checksum OK";
    else
        MTL_LOG(Info) << "[ACPI] Table " << table.GetSignature() << " found at " << mtl::hex(address) << ", Checksum FAILED";
}

template <typename T>
static void AcpiLogTables(const T& rootTable)
{
    for (auto address : rootTable)
    {
        const auto table = AcpiMapTable(address);
        AcpiLogTable(*table, address);

        if (table->GetSignature() == std::string_view("FACP"))
        {
            const auto fadt = static_cast<const AcpiFadt*>(table);
            const PhysicalAddress dsdtAddress = AcpiTableContains(fadt, X_DSDT) ? fadt->X_DSDT : fadt->DSDT;
            const auto dsdt = AcpiMapTable(dsdtAddress);
            AcpiLogTable(*dsdt, dsdtAddress);
        }
    }
}

void AcpiInitialize(const BootInfo& bootInfo)
{
    const auto& rsdp = *reinterpret_cast<const AcpiRsdp*>(bootInfo.acpiRsdp);

    MTL_LOG(Info) << "[ACPI] ACPI revision " << (int)rsdp.revision;

    // We are going to map all ACPI memory at a fixed offset from the physical memory.
    // This simplifies things quite a bit as we won't have to map ACPI tables as we want to use them.
    auto descriptors = (const efi::MemoryDescriptor*)bootInfo.memoryMap;
    auto descriptorCount = bootInfo.memoryMapLength;
    for (size_t i = 0; i != descriptorCount; ++i)
    {
        const auto& descriptor = descriptors[i];
        if (descriptor.type == efi::MemoryType::AcpiReclaimable || descriptor.type == efi::MemoryType::AcpiNonVolatile)
        {
            const auto virtualAddress = (void*)(uintptr_t)(descriptor.physicalStart + kAcpiMemoryOffset);

            uint64_t pageFlags;

            if (descriptor.type == efi::MemoryType::AcpiReclaimable)
            {
                pageFlags = mtl::PageFlags::KernelData_RO;
            }
            else
            {
                static_assert(mtl::PageFlags::WriteBack == 0);

                pageFlags = mtl::PageFlags::KernelData_RW;

                if (descriptor.attributes & efi::MemoryAttribute::WriteBack)
                    pageFlags |= mtl::PageFlags::WriteBack;
                else if (descriptor.attributes & efi::MemoryAttribute::WriteCombining)
                    pageFlags |= mtl::PageFlags::WriteCombining;
                else if (descriptor.attributes & efi::MemoryAttribute::WriteThrough)
                    pageFlags |= mtl::PageFlags::WriteThrough;
                else if (descriptor.attributes & efi::MemoryAttribute::Uncacheable)
                    pageFlags |= mtl::PageFlags::Uncacheable;
                else
                {
                    // TODO: we are supposed to fallback on ACPI memory descriptors for cacheability attributes, see UEFI 2.3.2
                    pageFlags |= mtl::PageFlags::Uncacheable;
                }
            }

            MapPages(descriptor.physicalStart, virtualAddress, descriptor.numberOfPages, (mtl::PageFlags)pageFlags);
            MTL_LOG(Info) << "[ACPI] Mapped ACPI memory: " << mtl::hex(descriptor.physicalStart) << " to " << virtualAddress
                          << ", page count " << descriptor.numberOfPages;
        }
    }

    // TODO: we need error handling here
    const auto rsdt = AcpiMapTable<AcpiRsdt>(rsdp.rsdtAddress);
    if (rsdt->VerifyChecksum())
        g_rsdt = rsdt;
    else
        MTL_LOG(Warning) << "[ACPI] RSDT checksum is invalid";

    if (rsdp.revision >= 2)
    {
        const auto xsdt = AcpiMapTable<AcpiXsdt>(static_cast<const AcpiRsdpExtended&>(rsdp).xsdtAddress);
        if (xsdt->VerifyChecksum())
            g_xsdt = xsdt;
        else
            MTL_LOG(Warning) << "[ACPI] XSDT checksum is invalid";
    }

    if (!g_rsdt && !g_xsdt)
    {
        MTL_LOG(Fatal) << "[ACPI] No valid ACPI root table found";
        abort();
    }

    if (g_xsdt)
        AcpiLogTables(*g_xsdt);
    else
        AcpiLogTables(*g_rsdt);

    lai_set_acpi_revision(rsdp.revision);
    lai_create_namespace();
}

void AcpiEnable(AcpiInterruptModel model)
{
    lai_enable_acpi(static_cast<uint32_t>(model));
}

void AcpiDisable()
{
    lai_disable_acpi();
}

const AcpiTable* AcpiFindTable(const char* signature, int index)
{
    return (const AcpiTable*)laihost_scan(signature, index);
}

void AcpiReset()
{
    lai_acpi_reset();
}

void AcpiSleep(AcpiSleepState state)
{
    lai_enter_sleep(static_cast<uint8_t>(state));
}
