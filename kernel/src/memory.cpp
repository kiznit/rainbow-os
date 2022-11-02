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

#include "memory.hpp"
#include "arch.hpp"
#include <metal/arch.hpp>
#include <metal/helpers.hpp>
#include <metal/log.hpp>
#include <vector>

static_assert(mtl::kMemoryPageSize == efi::kPageSize);

static std::vector<efi::MemoryDescriptor> g_systemMemoryMap;

static void InitializeFirmware(efi::RuntimeServices* runtimeServices)
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
            MTL_LOG(Fatal) << "[KRNL] Unable to determine page flags for memory at " << mtl::hex(descriptor.physicalStart);
            std::abort();
        }

        const auto virtualAddress = ArchMapSystemMemory(descriptor.physicalStart, descriptor.numberOfPages, pageFlags);
        if (!virtualAddress)
        {
            MTL_LOG(Fatal) << "[KRNL] Unable to map system memory at " << mtl::hex(descriptor.physicalStart);
            std::abort();
        }

        descriptor.virtualStart = reinterpret_cast<uintptr_t>(virtualAddress.value());
    }

    const auto status = runtimeServices->SetVirtualAddressMap(firmwareMemory.size() * sizeof(efi::MemoryDescriptor),
                                                              sizeof(efi::MemoryDescriptor), 1, firmwareMemory.data());

    if (efi::Error(status))
    {
        MTL_LOG(Fatal) << "[KRNL] Call to UEFI's SetVirtualAddressMap failed with " << mtl::hex(status);
        std::abort();
    }

    MTL_LOG(Info) << "[KRNL] UEFI Runtime set to virtual mode";
}

static void Log(const std::vector<efi::MemoryDescriptor>& memoryMap)
{
    MTL_LOG(Info) << "[KRNL] System memory map:";

    for (const auto& descriptor : memoryMap)
    {
        MTL_LOG(Info) << "[KRNL] " << mtl::hex(descriptor.physicalStart) << " - "
                      << mtl::hex(descriptor.physicalStart + descriptor.numberOfPages * mtl::kMemoryPageSize - 1) << ": "
                      << efi::ToString(descriptor.type);
    }
}

static void Tidy(std::vector<efi::MemoryDescriptor>& memoryMap)
{
    std::sort(memoryMap.begin(), memoryMap.end(), [](const efi::MemoryDescriptor& a, const efi::MemoryDescriptor& b) -> bool {
        return a.physicalStart < b.physicalStart;
    });

    // Merge adjacent memory descriptors
    size_t lastIndex = 0;
    for (size_t i = 1; i != memoryMap.size(); ++i)
    {
        const auto& descriptor = memoryMap[i];

        // See if we can merge the descriptor with the last entry
        auto& last = memoryMap[lastIndex];
        if (descriptor.type == last.type && descriptor.attributes == last.attributes &&
            descriptor.physicalStart == last.physicalStart + last.numberOfPages * mtl::kMemoryPageSize)
        {
            // Extend last entry instead of creating a new one
            last.numberOfPages += descriptor.numberOfPages;
            continue;
        }

        memoryMap[++lastIndex] = descriptor;
    }

    memoryMap.resize(lastIndex + 1);
}

static void FreeBootMemory()
{
    ArchReleaseBootMemory();

    for (auto& descriptor : g_systemMemoryMap)
    {
        if (descriptor.type == efi::MemoryType::BootServicesCode || descriptor.type == efi::MemoryType::BootServicesData ||
            descriptor.type == efi::MemoryType::LoaderCode || descriptor.type == efi::MemoryType::LoaderData)
        {
            descriptor.type = efi::MemoryType::Conventional;
        }
    }
}

void MemoryInitialize(efi::RuntimeServices* runtimeServices, std::vector<efi::MemoryDescriptor> memoryMap)
{
    g_systemMemoryMap = std::move(memoryMap);

    // Make sure to call UEFI's SetVirtualMemoryMap() while we have the UEFI boot services still mapped in the lower 4 GB.
    // This is to work around buggy runtime services that call into boot services during a call to SetVirtualMemoryMap().
    InitializeFirmware(runtimeServices);

    // It is now save to release boot services data
    FreeBootMemory();

    Tidy(g_systemMemoryMap);
    Log(g_systemMemoryMap);
}

const efi::MemoryDescriptor* MemoryFindSystemDescriptor(PhysicalAddress address)
{
    const auto descriptor =
        std::find_if(g_systemMemoryMap.begin(), g_systemMemoryMap.end(), [address](const efi::MemoryDescriptor& descriptor) {
            return address >= descriptor.physicalStart &&
                   address - descriptor.physicalStart <= descriptor.numberOfPages * mtl::kMemoryPageSize;
        });
    return descriptor != g_systemMemoryMap.end() ? descriptor : nullptr;
}

// TODO: support for non-contiguous frames
std::expected<PhysicalAddress, ErrorCode> AllocFrames(size_t pageCount)
{
    efi::MemoryDescriptor* candidate{};

    for (auto& descriptor : g_systemMemoryMap)
    {
        if (descriptor.type != efi::MemoryType::Conventional)
            continue;

        if (!(descriptor.attributes & efi::MemoryAttribute::WriteBack))
            continue;

        if (descriptor.numberOfPages < pageCount)
            continue;

        if (!candidate || descriptor.physicalStart > candidate->physicalStart)
            candidate = &descriptor;
    }

    if (!candidate)
        return std::unexpected(ErrorCode::OutOfMemory);

    // We have to be careful about recursion here, which can happen when growing the vector of memory descriptors.
    // Consider what would happen if:
    //      1) 'candidate' is the only block of available memory and
    //      2) the vector needs to grow
    const auto address = candidate->physicalStart + (candidate->numberOfPages - pageCount) * mtl::kMemoryPageSize;
    candidate->numberOfPages -= pageCount;

    // Track the newly allocated memory
    for (auto& descriptor : g_systemMemoryMap)
    {
        if (descriptor.type != efi::MemoryType::KernelData || descriptor.attributes != candidate->attributes)
            continue;

        // Is the entry adjacent?
        if (address == descriptor.physicalStart + descriptor.numberOfPages * mtl::kMemoryPageSize)
        {
            descriptor.numberOfPages += pageCount;
            return address;
        }

        if (address + pageCount * mtl::kMemoryPageSize == descriptor.physicalStart)
        {
            descriptor.physicalStart = address;
            descriptor.numberOfPages += pageCount;
            return address;
        }
    }

    // We must create a new descriptor
    g_systemMemoryMap.emplace_back(efi::MemoryDescriptor{.type = efi::MemoryType::KernelData,
                                                         .padding = 0,
                                                         .physicalStart = address,
                                                         .virtualStart = 0,
                                                         .numberOfPages = pageCount,
                                                         .attributes = candidate->attributes});

    return address;
}

std::expected<void, ErrorCode> FreeFrames(PhysicalAddress frames, size_t pageCount)
{
    // TODO
    (void)frames;
    (void)pageCount;

    return {};
}

std::expected<void, ErrorCode> VirtualAlloc(void* address, size_t size)
{
    // TODO: need some validation that [address, address + size] is not already mapped
    size = mtl::AlignUp(size, mtl::kMemoryPageSize);

    const auto pageCount = size >> mtl::kMemoryPageShift;

    // TODO: support for non-contiguous frames
    const auto frames = AllocFrames(pageCount);
    if (!frames)
        return std::unexpected(frames.error());

    auto result = MapPages(frames.value(), address, pageCount, mtl::PageFlags::KernelData_RW);
    if (!result)
        return std::unexpected(result.error());

    return {};
}

std::expected<void, ErrorCode> VirtualFree(void* address, size_t size)
{
    // TODO
    (void)address;
    (void)size;

    return {};
}

mtl::PageFlags MemoryGetPageFlags(const efi::MemoryDescriptor& descriptor)
{
    uint64_t pageFlags;

    if (descriptor.type == efi::MemoryType::BootServicesCode || descriptor.type == efi::MemoryType::RuntimeServicesCode)
        pageFlags = mtl::PageFlags::KernelCode;
    else
        pageFlags = mtl::PageFlags::KernelData_RW;

    static_assert(mtl::PageFlags::WriteBack == 0);
    if (descriptor.attributes & efi::MemoryAttribute::WriteBack)
        pageFlags |= mtl::PageFlags::WriteBack;
    else if (descriptor.attributes & efi::MemoryAttribute::WriteCombining)
        pageFlags |= mtl::PageFlags::WriteCombining;
    else if (descriptor.attributes & efi::MemoryAttribute::WriteThrough)
        pageFlags |= mtl::PageFlags::WriteThrough;
    else if (descriptor.attributes & efi::MemoryAttribute::Uncacheable)
        pageFlags |= mtl::PageFlags::Uncacheable;
    else
        pageFlags = 0;

    return (mtl::PageFlags)pageFlags;
}
