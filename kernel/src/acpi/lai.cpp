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

#include "lai.hpp"
#include "memory.hpp"
#include <cstdlib>
#include <lai/host.h>
#include <metal/log.hpp>
#include <rainbow/acpi.hpp>

extern acpi::Rsdt* g_rsdt;
extern acpi::Xsdt* g_xsdt;

void* laihost_malloc(size_t size)
{
    return malloc(size);
}

void* laihost_realloc(void* oldptr, size_t newsize, size_t /*oldsize*/)
{
    return realloc(oldptr, newsize);
}

void laihost_free(void* ptr, size_t /*size*/)
{
    return free(ptr);
}

void laihost_log(int level, const char* message)
{
    if (level == LAI_DEBUG_LOG)
        MTL_LOG(Debug) << "[LAI] " << message;
    else
        MTL_LOG(Warning) << "[LAI] " << message;
}

void* laihost_map(size_t address, size_t count)
{
    if (auto memory = MapMemory(address, count, mtl::PageFlags::KernelData_RW))
    {
        // MTL_LOG(Debug) << "laihost_map: mapped " << mtl::hex(address) << " to " << memory.value() << " for " << count << "
        // bytes";
        return memory.value();
    }
    else
        return nullptr;
}

__attribute__((noreturn)) void laihost_panic(const char* message)
{
    MTL_LOG(Fatal) << "[LAI] " << message;
    abort();
}

template <typename T>
static const acpi::Table* laihost_scan(const T& rootTable, std::string_view signature, int index)
{
    int count = 0;
    for (auto address : rootTable)
    {
        const auto table = AcpiMapTable(address);
        if (table->GetSignature() == signature)
        {
            if (index == count)
                return table;

            ++count;
        }
    }

    MTL_LOG(Warning) << "laihost_scan(): table " << signature << " not found";
    return nullptr;
}

void* laihost_scan(const char* signature_, size_t index)
{
    const std::string_view signature(signature_);

    if (signature == std::string_view("DSDT"))
    {
        const auto facp = (acpi::Fadt*)laihost_scan("FACP", 0);
        if (!facp)
            return nullptr;

        PhysicalAddress dsdtAddress = facp->DSDT;
        if (facp->length >= 148 && facp->X_DSDT)
            dsdtAddress = facp->X_DSDT;

        return (void*)AcpiMapTable(dsdtAddress);
    }

    if (g_xsdt)
        return (void*)laihost_scan(*g_xsdt, signature, index);
    else
        return (void*)laihost_scan(*g_rsdt, signature, index);
}
