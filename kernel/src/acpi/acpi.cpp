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

const acpi::Rsdt* g_rsdt{};
const acpi::Xsdt* g_xsdt{};

void AcpiInitialize(const acpi::Rsdp& rsdp)
{
    MTL_LOG(Info) << "ACPI revision " << (int)rsdp.revision;

    // TODO: we need error handling here
    const auto rsdt = AcpiMapTable<acpi::Rsdt>(rsdp.rsdtAddress);
    if (rsdt->VerifyChecksum())
        g_rsdt = rsdt;
    else
        MTL_LOG(Warning) << "RSDT checksum is invalid";

    if (rsdp.revision >= 2)
    {
        const auto xsdt = AcpiMapTable<acpi::Xsdt>(static_cast<const acpi::RsdpExtended&>(rsdp).xsdtAddress);
        if (xsdt->VerifyChecksum())
            g_xsdt = xsdt;
        else
            MTL_LOG(Warning) << "XSDT checksum is invalid";
    }

    if (!g_rsdt && !g_xsdt)
    {
        MTL_LOG(Fatal) << "No valid ACPI root table found";
        abort();
    }

    lai_set_acpi_revision(rsdp.revision);
    lai_create_namespace();
}

void AcpiEnable(AcpiInterruptModel model)
{
    lai_enable_acpi(static_cast<uint32_t>(model));
}

void AcpiDisable()
{
}

void AcpiReset()
{
    lai_acpi_reset();
}

void AcpiSleep(AcpiSleepState state)
{
    lai_enter_sleep(static_cast<uint8_t>(state));
}
