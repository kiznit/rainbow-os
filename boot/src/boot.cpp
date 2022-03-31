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

#include "boot.hpp"
#include "MemoryMap.hpp"
#include "VirtualMemory.hpp"
#include "elf.hpp"
#include "uefi.hpp"
#include <metal/helpers.hpp>
#include <metal/log.hpp>

extern efi::BootServices* g_efiBootServices;

static MemoryMap* g_memoryMap;

std::expected<mtl::PhysicalAddress, efi::Status> AllocatePages(size_t pageCount)
{
    if (g_efiBootServices)
    {
        efi::PhysicalAddress memory{0};
        const auto status = g_efiBootServices->AllocatePages(
            efi::AllocateAnyPages, efi::EfiLoaderData, pageCount, &memory);

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

std::expected<void, efi::Status> Boot()
{
    InitializeDisplays();

    auto kernel = LoadModule("kernel");
    if (!kernel)
    {
        MTL_LOG(Fatal) << "Failed to load kernel image: " << mtl::hex(kernel.error());
        return std::unexpected(kernel.error());
    }

    MTL_LOG(Info) << "Kernel size: " << kernel->size << " bytes";

    VirtualMemory vmm;
    if (!elf_load(*kernel, vmm))
    {
        MTL_LOG(Fatal) << "Failed to load kernel module";
        return std::unexpected(efi::LoadError);
    }

    auto memoryMap = ExitBootServices();
    if (!memoryMap)
    {
        return std::unexpected(memoryMap.error());
    }

    g_memoryMap = memoryMap.value();

    // Once we have exited boot services, we can never return

    for (;;) {}
}