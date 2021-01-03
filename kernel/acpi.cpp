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

#include "acpi.hpp"
#include <metal/log.hpp>


static const Acpi::Rsdp20* rsdp;
static const Acpi::Rsdt* rsdt;
static const Acpi::Xsdt* xsdt;


void acpi_init(uint64_t rsdtAddress)
{
    if (!rsdtAddress)
    {
        Log("acpi_init(): ACPI not available\n");
        return;
    }

    rsdp = (const Acpi::Rsdp20*)rsdtAddress;

    if (rsdp->revision >= 2 && rsdp->xsdtAddress)
    {
        // TODO: this doesn't work if we are running 32 bits and xsdtAddress > 4 GB.
        // What we need to do is map the ACPI data in virtual memory space.
        xsdt = (Acpi::Xsdt*)rsdp->xsdtAddress;

        // TODO: verify checksum?
    }
    else
    {
        rsdt = (Acpi::Rsdt*)(uintptr_t)rsdp->rsdtAddress;

        // TODO: verify checksum?
    }
}


const Acpi::Table* acpi_find_table(uint32_t signature)
{
    if (xsdt)
    {
        for (const uint64_t* it = xsdt->tables; (uintptr_t)it < (uintptr_t)xsdt + xsdt->length; ++it)
        {
            const auto table = (Acpi::Table*)(uintptr_t)*it;
            if (table->signature == signature)
            {
                // TODO: verify checksum of table
                return table;
            }
        }
    }
    else if (rsdt)
    {
        for (const uint32_t* it = rsdt->tables; (uintptr_t)it < (uintptr_t)rsdt + rsdt->length; ++it)
        {
            const auto table = (Acpi::Table*)(uintptr_t)*it;
            if (table->signature == signature)
            {
                // TODO: verify checksum of table
                return table;
            }
        }
    }

    return nullptr;
}