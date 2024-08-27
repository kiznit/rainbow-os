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

#include "uefi.hpp"
#include "arch.hpp"
#include "memory.hpp"
#include <metal/log.hpp>
#include <rainbow/acpi.hpp>

extern std::vector<efi::MemoryDescriptor> g_systemMemoryMap;

static const efi::SystemTable* g_efiSystemTable{};

static void UefiSetVirtualMemoryMap(const efi::SystemTable& systemTable)
{
    std::vector<efi::MemoryDescriptor> firmwareMemory;
    for (const auto& descriptor : g_systemMemoryMap)
    {
        // We also map ACPI memory here as it is convenient
        if ((descriptor.attributes & efi::MemoryAttribute::Runtime) || descriptor.type == efi::MemoryType::AcpiReclaimable ||
            descriptor.type == efi::MemoryType::AcpiNonVolatile)
        {
            firmwareMemory.emplace_back(descriptor);
        }
    }

    for (auto& descriptor : firmwareMemory)
    {
        const auto pageFlags = MemoryGetPageFlags(descriptor);
        if (pageFlags == 0)
        {
            // This happens on my Intel NUC where "type" is "Reserved" and "attributes" is "Runtime" (and nothing else).
            MTL_LOG(Warning) << "[KRNL] UefiSetVirtualMemoryMap(): unable to determine page flags for memory at "
                             << mtl::hex(descriptor.physicalStart) << ", type: " << (int)descriptor.type
                             << ", attributes: " << mtl::hex(descriptor.attributes);
            continue;
        }

        const auto virtualAddress = ArchMapSystemMemory(descriptor.physicalStart, descriptor.numberOfPages, pageFlags);
        if (!virtualAddress)
        {
            MTL_LOG(Fatal) << "[KRNL] Unable to map system memory at " << mtl::hex(descriptor.physicalStart);
            std::abort();
        }

        descriptor.virtualStart = reinterpret_cast<uintptr_t>(virtualAddress.value());
    }

    const auto status = systemTable.runtimeServices->SetVirtualAddressMap(firmwareMemory.size() * sizeof(efi::MemoryDescriptor),
                                                                          sizeof(efi::MemoryDescriptor), 1, firmwareMemory.data());

    if (efi::Error(status))
    {
        MTL_LOG(Fatal) << "[KRNL] Call to UEFI's SetVirtualAddressMap failed with " << mtl::hex(status);
        std::abort();
    }

    // Invalidate functions we cannot use anymore
    systemTable.runtimeServices->SetVirtualAddressMap = nullptr;
    systemTable.runtimeServices->ConvertPointer = nullptr;

    // Fix configuration tables, SetVirtualAddressMap() doesn't do it
    for (unsigned int i = 0; i != systemTable.numberOfTableEntries; ++i)
    {
        auto& table = systemTable.configurationTable[i];
        table.vendorTable = (void*)ArchGetSystemMemory((uintptr_t)table.vendorTable);
    }

    MTL_LOG(Info) << "[KRNL] UEFI Runtime set to virtual mode";
}

void UefiInitialize(const efi::SystemTable& systemTable)
{
    UefiSetVirtualMemoryMap(systemTable);

    g_efiSystemTable = (efi::SystemTable*)ArchGetSystemMemory((uintptr_t)&systemTable);
}

const AcpiRsdp* UefiFindAcpiRsdp()
{
    const AcpiRsdp* result = nullptr;

    for (unsigned int i = 0; i != g_efiSystemTable->numberOfTableEntries; ++i)
    {
        const auto& table = g_efiSystemTable->configurationTable[i];

        // ACPI 1.0
        if (table.vendorGuid == efi::kAcpi1TableGuid)
        {
            if (auto rsdp = (const AcpiRsdp*)table.vendorTable)
            {
                if (rsdp->VerifyChecksum())
                    result = rsdp;
                else
                    MTL_LOG(Warning) << "[UEFI] RSDP has invalid checksum, ignoring";
            }
        }

        // ACPI 2.0
        if (table.vendorGuid == efi::kAcpi2TableGuid)
        {
            if (auto rsdp = (const AcpiRsdpExtended*)table.vendorTable)
            {
                if (rsdp->VerifyExtendedChecksum())
                {
                    result = rsdp;
                    break;
                }

                MTL_LOG(Warning) << "[UEFI] Extended RSDP has invalid checksum, ignoring";
            }
        }
    }

    return result;
}

const DeviceTree* UefiFindDeviceTree()
{
    for (unsigned int i = 0; i != g_efiSystemTable->numberOfTableEntries; ++i)
    {
        const auto& table = g_efiSystemTable->configurationTable[i];
        if (table.vendorGuid == efi::kFdtTableGuid)
        {
            return static_cast<const DeviceTree*>(table.vendorTable);
        }
    }

    return nullptr;
}
