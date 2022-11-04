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
#include <lai/helpers/pm.h>
#include <lai/helpers/sci.h>
#include <metal/arch.hpp>
#include <metal/log.hpp>

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

void AcpiInitialize(const AcpiRsdp& rsdp)
{
    if (rsdp.revision >= 2 && static_cast<const AcpiRsdpExtended&>(rsdp).xsdtAddress)
    {
        g_xsdt = AcpiMapTable<AcpiXsdt>(static_cast<const AcpiRsdpExtended&>(rsdp).xsdtAddress);
        MTL_LOG(Info) << "[ACPI] Using ACPI XSDT with revision " << (int)rsdp.revision;
    }
    else if (rsdp.rsdtAddress)
    {
        g_rsdt = AcpiMapTable<AcpiRsdt>(rsdp.rsdtAddress);
        MTL_LOG(Info) << "[ACPI] Using ACPI RSDT with revision " << (int)rsdp.revision;
    }
    else
    {
        MTL_LOG(Fatal) << "[ACPI] No ACPI RSDP table found";
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
