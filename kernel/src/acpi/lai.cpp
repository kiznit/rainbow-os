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
#include "acpi.hpp"
#include "memory.hpp"
#include "pci.hpp"
#include <cstdlib>
#include <lai/host.h>
#include <metal/helpers.hpp>
#include <metal/log.hpp>
#include <rainbow/acpi.hpp>

#if __x86_64__
#include <metal/arch.hpp>
#endif

extern AcpiRsdt* g_rsdt;
extern AcpiXsdt* g_xsdt;

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
        MTL_LOG(Debug) << "[ACPI] " << message;
    else
        MTL_LOG(Warning) << "[ACPI] " << message;
}

void* laihost_map(size_t address, size_t count)
{
    // We need to find the memory descriptor for the specified address to determine what type of caching to use.
    const auto descriptor = MemoryFindSystemDescriptor(address);

    mtl::PageFlags pageFlags = descriptor ? AcpiGetPageFlags(*descriptor) : (mtl::PageFlags)0;
    if (pageFlags == 0)
        // TODO: we are supposed to fallback on ACPI memory descriptors for cacheability attributes, see UEFI 2.3.2
        pageFlags = mtl::PageFlags::MMIO;

    const auto startAddress = mtl::AlignDown(address, mtl::kMemoryPageSize);
    const auto endAddress = mtl::AlignUp(address + count, mtl::kMemoryPageSize);
    const auto pageCount = (endAddress - startAddress) >> mtl::kMemoryPageShift;

    assert(startAddress <= 0x0000800000000000ull);
    assert(endAddress <= 0x0000800000000000ull);

    if (auto virtualAddress = ArchMapSystemMemory(startAddress, pageCount, pageFlags))
        return mtl::AdvancePointer(*virtualAddress, address - startAddress);
    else
        return nullptr; // TODO: report the error
}

uint8_t laihost_inb(uint16_t port)
{
#if __x86_64__
    return mtl::x86_inb(port);
#else
    (void)port;
    lai_panic("i/o port not implemented");
#endif
}

uint16_t laihost_inw(uint16_t port)
{
#if __x86_64__
    return mtl::x86_inw(port);
#else
    (void)port;
    lai_panic("i/o port not implemented");
#endif
}

uint32_t laihost_ind(uint16_t port)
{
#if __x86_64__
    return mtl::x86_inl(port);
#else
    (void)port;
    lai_panic("i/o port not implemented");
#endif
}

void laihost_outb(uint16_t port, uint8_t value)
{
#if __x86_64__
    mtl::x86_outb(port, value);
#else
    (void)port;
    (void)value;
    lai_panic("i/o port not implemented");
#endif
}

void laihost_outw(uint16_t port, uint16_t value)
{
#if __x86_64__
    mtl::x86_outw(port, value);
#else
    (void)port;
    (void)value;
    lai_panic("i/o port not implemented");
#endif
}

void laihost_outd(uint16_t port, uint32_t value)
{
#if __x86_64__
    mtl::x86_outl(port, value);
#else
    (void)port;
    (void)value;
    lai_panic("i/o port not implemented");
#endif
}

__attribute__((noreturn)) void laihost_panic(const char* message)
{
    MTL_LOG(Fatal) << "[ACPI] " << message;
    abort();
}

void laihost_pci_writeb(uint16_t segment, uint8_t bus, uint8_t slot, uint8_t function, uint16_t offset, uint8_t value)
{
    PciWrite8(segment, bus, slot, function, offset, value);
}

void laihost_pci_writew(uint16_t segment, uint8_t bus, uint8_t slot, uint8_t function, uint16_t offset, uint16_t value)
{
    PciWrite16(segment, bus, slot, function, offset, value);
}

void laihost_pci_writed(uint16_t segment, uint8_t bus, uint8_t slot, uint8_t function, uint16_t offset, uint32_t value)
{
    PciWrite32(segment, bus, slot, function, offset, value);
}

uint8_t laihost_pci_readb(uint16_t segment, uint8_t bus, uint8_t slot, uint8_t function, uint16_t offset)
{
    return PciRead8(segment, bus, slot, function, offset);
}

uint16_t laihost_pci_readw(uint16_t segment, uint8_t bus, uint8_t slot, uint8_t function, uint16_t offset)
{
    return PciRead16(segment, bus, slot, function, offset);
}

uint32_t laihost_pci_readd(uint16_t segment, uint8_t bus, uint8_t slot, uint8_t function, uint16_t offset)
{
    return PciRead32(segment, bus, slot, function, offset);
}

template <typename T>
static const AcpiTable* laihost_scan(const T& rootTable, std::string_view signature, int index)
{
    int count = 0;
    for (auto address : rootTable)
    {
        const auto table = AcpiMapTable(address);
        if (table->GetSignature() == signature)
        {
            if (table->VerifyChecksum())
            {
                if (index == count)
                    return table;

                ++count;
            }
            else
                MTL_LOG(Warning) << "[ACPI] " << signature << " checksum is invalid in laihost_scan()";
        }
    }

    return nullptr;
}

void* laihost_scan(const char* signature_, size_t index)
{
    const std::string_view signature(signature_);

    if (signature == std::string_view("DSDT"))
    {
        const auto fadt = (AcpiFadt*)laihost_scan("FACP", 0);
        if (!fadt)
            return nullptr;

        const PhysicalAddress dsdtAddress = AcpiTableContains(fadt, X_DSDT) ? fadt->X_DSDT : fadt->DSDT;

        return (void*)AcpiMapTable(dsdtAddress);
    }

    if (g_xsdt)
        return (void*)laihost_scan(*g_xsdt, signature, index);
    else
        return (void*)laihost_scan(*g_rsdt, signature, index);
}

void laihost_sleep(uint64_t milliseconds)
{
    // TODO: implement
    (void)milliseconds;
    MTL_LOG(Warning) << "[ACPI] laihost_sleep() not implemented (" << milliseconds << " ms)";
    // assert(0);
}

uint64_t laihost_timer()
{
    // TODO: implement
    assert(0);
    return 0;
}
