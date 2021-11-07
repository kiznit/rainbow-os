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
#include "EfiConsole.hpp"
#include "MemoryMap.hpp"
#include <metal/log.hpp>
#include <vector>

efi::Handle g_efiImage;
efi::SystemTable* g_efiSystemTable;
efi::BootServices* g_efiBootServices;
efi::RuntimeServices* g_efiRuntimeServices;

// TODO: we'd like to return a smart pointer here, don't we?
metal::expected<MemoryMap*, efi::Status> ExitBootServices()
{
    efi::uintn_t bufferSize = 0;
    efi::MemoryDescriptor* descriptors = nullptr;
    efi::uintn_t memoryMapKey = 0;
    efi::uintn_t descriptorSize = 0;
    uint32_t descriptorVersion = 0;

    // 1) Retrieve the memory map from the firmware
    efi::Status status;
    std::vector<char> buffer;
    while ((status = g_efiBootServices->GetMemoryMap(&bufferSize, descriptors, &memoryMapKey,
                                                     &descriptorSize, &descriptorVersion)) ==
           efi::BufferTooSmall)
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
    }

    if (efi::Error(status))
    {
        METAL_LOG(Fatal) << u8"Failed to retrieve the EFI memory map (1): " << (void*)status;
        return metal::unexpected(status);
    }

    // 2) Exit boot services - it is possible for the firmware to modify the memory map
    // during a call to ExitBootServices(). A so-called "partial shutdown".
    // When that happens, ExitBootServices() will return EFI_INVALID_PARAMETER.
    while ((status = g_efiBootServices->ExitBootServices(g_efiImage, memoryMapKey)) ==
           efi::InvalidParameter)
    {
        // Memory map changed during ExitBootServices(), the only APIs we are allowed to
        // call at this point are GetMemoryMap() and ExitBootServices().
        bufferSize = buffer.size();
        status = g_efiBootServices->GetMemoryMap(&bufferSize, descriptors, &memoryMapKey,
                                                 &descriptorSize, &descriptorVersion);
        if (efi::Error(status))
        {
            METAL_LOG(Fatal) << u8"Failed to retrieve the EFI memory map (2): " << (void*)status;
            return metal::unexpected(status);
        }
    }

    if (efi::Error(status))
    {
        METAL_LOG(Fatal) << u8"Failed to exit boot services: " << (void*)status;
        return metal::unexpected(status);
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

    // NOTE: we exited boot services and don't have a memory map to do allocations.
    // So how does this work? In most cases we are lucky and the heap implementation
    // is able to allocate the memory we need from chunks already acquired with mmap().
    // If we are unlucky, mmap() is called when we try to allocate and build the memory
    // map. The way we work around this is by pre-allocating a chunk of memory for this
    // emergency state. See malloc.cpp for the details.

    return new MemoryMap(descriptors, bufferSize / descriptorSize, descriptorSize);
}

static void PrintBanner(efi::SimpleTextOutputProtocol* console)
{
    console->SetAttribute(console, efi::BackgroundBlack);
    console->ClearScreen(console);

    console->SetAttribute(console, efi::Red);
    console->OutputString(console, L"R");
    console->SetAttribute(console, efi::LightRed);
    console->OutputString(console, L"a");
    console->SetAttribute(console, efi::Yellow);
    console->OutputString(console, L"i");
    console->SetAttribute(console, efi::LightGreen);
    console->OutputString(console, L"n");
    console->SetAttribute(console, efi::LightCyan);
    console->OutputString(console, L"b");
    console->SetAttribute(console, efi::LightBlue);
    console->OutputString(console, L"o");
    console->SetAttribute(console, efi::LightMagenta);
    console->OutputString(console, L"w");
    console->SetAttribute(console, efi::LightGray);

    console->OutputString(console, L" UEFI bootloader\n\r\n\r");
}

// Cannot use "main()" as the function name as this causes problems with mingw
efi::Status efi_main()
{
    const auto conOut = g_efiSystemTable->conOut;
    PrintBanner(conOut);

    EfiConsole console(conOut);
    metal::g_log.AddLogger(&console);

    METAL_LOG(Info) << u8"UEFI firmware vendor: " << g_efiSystemTable->firmwareVendor;
    METAL_LOG(Info) << u8"UEFI firmware revision: " << (g_efiSystemTable->firmwareRevision >> 16)
                    << u8'.' << (g_efiSystemTable->firmwareRevision & 0xFFFF);

    auto memoryMap = ExitBootServices();
    if (!memoryMap)
    {
        // EFI doesn't specify a generic error code and none of the existing
        // error codes seems to make sense here. So we just go with "Unsupported".
        return efi::Unsupported;
    }

    // Once we have exited boot services, we can never return

    for (;;) {}
}
