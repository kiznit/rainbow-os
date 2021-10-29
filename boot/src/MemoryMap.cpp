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

#include "MemoryMap.hpp"

MemoryMap::MemoryMap(const efi::MemoryDescriptor* descriptors, size_t descriptorCount,
                     size_t descriptorSize)
{
    auto descriptor = descriptors;
    for (efi::uintn_t i = 0; i != descriptorCount;
         ++i, descriptor = (efi::MemoryDescriptor*)((uintptr_t)descriptor + descriptorSize))
    {
        MemoryType type;

        switch (descriptor->type)
        {
        case efi::EfiLoaderCode:
        case efi::EfiBootServicesCode:
            type = MemoryType::Bootloader;
            break;

        case efi::EfiLoaderData:
        case efi::EfiBootServicesData:
            type = MemoryType::Bootloader;
            break;

        case efi::EfiRuntimeServicesCode:
            type = MemoryType::UefiCode;
            break;

        case efi::EfiRuntimeServicesData:
            type = MemoryType::UefiData;
            break;

        case efi::EfiConventionalMemory:
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

        case efi::EfiUnusableMemory:
            type = MemoryType::Unusable;
            break;

        case efi::EfiACPIReclaimMemory:
            type = MemoryType::AcpiReclaimable;
            break;

        case efi::EfiACPIMemoryNVS:
            type = MemoryType::AcpiNvs;
            break;

        case efi::EfiPersistentMemory:
            type = MemoryType::Persistent;
            break;

        case efi::EfiReservedMemoryType:
        case efi::EfiMemoryMappedIO:
        case efi::EfiMemoryMappedIOPortSpace:
        case efi::EfiPalCode:
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

        SetMemoryRange(descriptor->physicalStart, descriptor->numberOfPages * efi::PageSize, type,
                       (MemoryFlags)flags);
    }
}

void MemoryMap::SetMemoryRange(PhysicalAddress address, PhysicalAddress sizeInBytes,
                               MemoryType type, MemoryFlags flags)
{
    (void)address;
    (void)sizeInBytes;
    (void)type;
    (void)flags;
}
