/*
    Copyright (c) 2021, Thierry Tremblay
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

efi::Handle g_efiImage;
efi::SystemTable* g_efiSystemTable;
efi::BootServices* g_efiBootServices;
efi::RuntimeServices* g_efiRuntimeServices;

static MemoryMap* g_memoryMap;

std::expected<mtl::PhysicalAddress, efi::Status> AllocatePages(size_t pageCount)
{
    if (g_efiBootServices)
    {
        efi::PhysicalAddress memory{0};
        const auto status = g_efiBootServices->AllocatePages(efi::AllocateAnyPages, efi::LoaderData, pageCount, &memory);

        if (!efi::Error(status))
            return memory;
    }

    if (g_memoryMap)
    {
        const auto memory = g_memoryMap->AllocatePages(MemoryType::Bootloader, pageCount);
        if (memory)
            return *memory;
    }

    return std::unexpected(efi::OutOfResource);
}

static void BuildMemoryMap(std::vector<MemoryDescriptor>& memoryMap, const efi::MemoryDescriptor* descriptors,
                           size_t descriptorCount, size_t descriptorSize)
{
    auto descriptor = descriptors;
    for (efi::uintn_t i = 0; i != descriptorCount;
         ++i, descriptor = (efi::MemoryDescriptor*)((uintptr_t)descriptor + descriptorSize))
    {
        MemoryType type;

        switch (descriptor->type)
        {
        case efi::LoaderCode:
        case efi::BootServicesCode:
            type = MemoryType::Bootloader;
            break;

        case efi::LoaderData:
        case efi::BootServicesData:
            type = MemoryType::Bootloader;
            break;

        case efi::RuntimeServicesCode:
            type = MemoryType::UefiCode;
            break;

        case efi::RuntimeServicesData:
            type = MemoryType::UefiData;
            break;

        case efi::ConventionalMemory:
            // Linux does this check... I am not sure how important it is... But let's do the same
            // for now. If memory isn't capable of "Writeback" caching, then it is not conventional
            // memory.
            if (descriptor->attribute & efi::MemoryWB)
            {
                type = MemoryType::Available;
            }
            else
            {
                type = MemoryType::Reserved;
            }
            break;

        case efi::UnusableMemory:
            type = MemoryType::Unusable;
            break;

        case efi::ACPIReclaimMemory:
            type = MemoryType::AcpiReclaimable;
            break;

        case efi::ACPIMemoryNVS:
            type = MemoryType::AcpiNvs;
            break;

        case efi::PersistentMemory:
            type = MemoryType::Persistent;
            break;

        case efi::ReservedMemoryType:
        case efi::MemoryMappedIO:
        case efi::MemoryMappedIOPortSpace:
        case efi::PalCode:
        default:
            type = MemoryType::Reserved;
            break;
        }

        // We assume that our flags match the EFI ones, so verify!
        static_assert((int)MemoryFlags::UC == efi::MemoryUC);
        static_assert((int)MemoryFlags::WC == efi::MemoryWC);
        static_assert((int)MemoryFlags::WT == efi::MemoryWT);
        static_assert((int)MemoryFlags::WB == efi::MemoryWB);
        static_assert((int)MemoryFlags::WP == efi::MemoryWP);
        static_assert((int)MemoryFlags::NV == efi::MemoryNV);

        uint32_t flags = descriptor->attribute & 0x7FFFFFFF;

        if (descriptor->attribute & efi::MemoryRuntime)
        {
            flags |= MemoryFlags::Runtime;
        }

        memoryMap.emplace_back(MemoryDescriptor{.type = type,
                                                .flags = (MemoryFlags)flags,
                                                .address = descriptor->physicalStart,
                                                .pageCount = descriptor->numberOfPages});
    }
}

std::expected<MemoryMap*, efi::Status> ExitBootServices()
{
    efi::uintn_t bufferSize = 0;
    efi::MemoryDescriptor* descriptors = nullptr;
    efi::uintn_t memoryMapKey = 0;
    efi::uintn_t descriptorSize = 0;
    uint32_t descriptorVersion = 0;
    std::vector<MemoryDescriptor> memoryMap;

    // 1) Retrieve the memory map from the firmware
    efi::Status status;
    std::vector<char> buffer;
    while ((status = g_efiBootServices->GetMemoryMap(&bufferSize, descriptors, &memoryMapKey, &descriptorSize,
                                                     &descriptorVersion)) == efi::BufferTooSmall)
    {
        // Add some extra space. There are few reasons for this:
        // a) Allocating memory for the buffer can increase the size of the memory map itself.
        //    Adding extra space will prevent an infinite loop.
        // b) We want to try to prevent a "partial shutdown" when calling ExitBootServices().
        //    See comment below about what a "partial shutdown" is.
        // c) If a "partial shutdown" does happen, we won't be able to allocate more memory!
        //    Having some extra space now should mitigate the issue.
        bufferSize += descriptorSize * 10;

        buffer.resize(bufferSize);
        descriptors = (efi::MemoryDescriptor*)buffer.data();

        // Allocate space for the memory map now as we can't do it after we exit boot services.
        memoryMap.reserve(bufferSize / descriptorSize);
    }

    if (efi::Error(status))
    {
        return std::unexpected(status);
    }

    // 2) Exit boot services - it is possible for the firmware to modify the memory map
    // during a call to ExitBootServices(). A so-called "partial shutdown".
    // When that happens, ExitBootServices() will return EFI_INVALID_PARAMETER.
    while ((status = g_efiBootServices->ExitBootServices(g_efiImage, memoryMapKey)) == efi::InvalidParameter)
    {
        // Memory map changed during ExitBootServices(), the only APIs we are allowed to
        // call at this point are GetMemoryMap() and ExitBootServices().
        bufferSize = buffer.size();
        status = g_efiBootServices->GetMemoryMap(&bufferSize, descriptors, &memoryMapKey, &descriptorSize, &descriptorVersion);
        if (efi::Error(status))
        {
            return std::unexpected(status);
        }
    }

    if (efi::Error(status))
    {
        return std::unexpected(status);
    }

    // Clear out fields we can't use anymore
    g_efiSystemTable->consoleInHandle = 0;
    g_efiSystemTable->conIn = nullptr;
    g_efiSystemTable->consoleOutHandle = 0;
    g_efiSystemTable->conOut = nullptr;
    g_efiSystemTable->standardErrorHandle = 0;
    g_efiSystemTable->stdErr = nullptr;
    g_efiSystemTable->bootServices = nullptr;

    g_efiBootServices = nullptr;

    // Note we can't allocate memory until g_memoryMap is set
    BuildMemoryMap(memoryMap, descriptors, bufferSize / descriptorSize, descriptorSize);

    return new MemoryMap(std::move(memoryMap));
}
