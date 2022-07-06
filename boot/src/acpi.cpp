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
#include <cassert>
#include <cstring>
#include <metal/log.hpp>

static void EnumeratePCI(const acpi::Mcfg& mcfg)
{
    for (const auto& config : mcfg)
    {
        MTL_LOG(Info) << "        " << mtl::hex(config.address) << ", segment: " << config.segment
                      << ", bus: " << int(config.startBus) << "-" << int(config.endBus);
    }
}

template <typename T>
static void EnumerateTablesImpl(const T& rootTable)
{
    if (!rootTable.VerifyChecksum())
    {
        MTL_LOG(Warning) << "    ACPI table checksum invalid, ignoring";
        return;
    }

    for (auto address : rootTable)
    {
        const auto table = reinterpret_cast<acpi::Table*>(address);
        MTL_LOG(Info) << "    " << table->GetSignature() << ", checksum: " << table->VerifyChecksum();

        if (0 == memcmp(table->GetSignature().data(), "MCFG", 4))
        {
            auto mcfg = static_cast<const acpi::Mcfg*>(table);
            EnumeratePCI(*mcfg);
        }
    }
}

void EnumerateTables(const acpi::Rsdp* rsdp)
{
    assert(rsdp != nullptr);

    if (rsdp->revision < 2)
    {
        MTL_LOG(Info) << "Enumerating RSDT";
        auto rsdt = reinterpret_cast<const acpi::Rsdt*>(rsdp->rsdt);
        EnumerateTablesImpl(*rsdt);
    }
    else
    {
        MTL_LOG(Info) << "Enumerating XSDT";
        auto rsdpExtended = static_cast<const acpi::RsdpExtended*>(rsdp);
        auto xsdt = reinterpret_cast<const acpi::Xsdt*>(rsdpExtended->xsdt);
        EnumerateTablesImpl(*xsdt);
    }
}
