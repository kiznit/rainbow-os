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
#include <metal/helpers.hpp>
#include <metal/log.hpp>
#include <rainbow/uefi/edid.hpp>
#include <rainbow/uefi/filesystem.hpp>
#include <rainbow/uefi/graphics.hpp>
#include <rainbow/uefi/image.hpp>
#include <vector>

efi::Handle g_efiImage;
efi::SystemTable* g_efiSystemTable;
efi::BootServices* g_efiBootServices;
efi::RuntimeServices* g_efiRuntimeServices;

static EfiConsole* s_efiLogger;
static efi::FileProtocol* g_fileSystem;

void InitDisplays()
{
    efi::uintn_t size{0};
    std::vector<efi::Handle> handles;
    efi::Status status;

    // LocateHandle() should only be called twice... But I don't want to write it twice :)
    while ((status = g_efiBootServices->LocateHandle(efi::ByProtocol,
                                                     &efi::GraphicsOutputProtocolGuid, nullptr,
                                                     &size, handles.data())) == efi::BufferTooSmall)
    {
        handles.resize(size / sizeof(efi::Handle));
    }

    if (efi::Error(status))
    {
        // Likely efi::NotFound, but any error should be handled as "no display available"
        METAL_LOG(Warning) << u8"Not UEFI displays found: " << (void*)status;
        return;
    }

    for (auto handle : handles)
    {
        efi::DevicePathProtocol* dpp = nullptr;
        if (efi::Error(g_efiBootServices->HandleProtocol(handle, &efi::DevicePathProtocolGuid,
                                                         (void**)&dpp)))
            continue;

        // If dpp is null, this is the "Console Splitter" driver. It is used to draw on all
        // screens at the same time and doesn't represent a real hardware device.
        if (!dpp)
            continue;

        efi::GraphicsOutputProtocol* gop{nullptr};
        if (efi::Error(g_efiBootServices->HandleProtocol(handle, &efi::GraphicsOutputProtocolGuid,
                                                         (void**)&gop)))
            continue;
        // gop is not expected to be null, but let's play safe.
        if (!gop)
            continue;

        efi::EdidProtocol* edid{nullptr};
        if (efi::Error(g_efiBootServices->HandleProtocol(handle, &efi::EdidActiveProtocolGuid,
                                                         (void**)&edid)) ||
            (!edid))
        {
            if (efi::Error(g_efiBootServices->HandleProtocol(
                    handle, &efi::EdidDiscoveredProtocolGuid, (void**)&edid)))
                edid = nullptr;
        }

        const auto& mode = *gop->Mode->info;
        METAL_LOG(Info) << u8"Display: " << mode.horizontalResolution << u8" x "
                        << mode.verticalResolution << u8", edid size: "
                        << (edid ? edid->sizeOfEdid : 0) << u8" bytes";
    }
}

static mtl::expected<efi::FileProtocol*, efi::Status> OpenRainbowDirectory()
{
    efi::Status status;

    efi::LoadedImageProtocol* image;
    status = g_efiBootServices->HandleProtocol(g_efiImage, &efi::LoadedImageProtocolGuid,
                                               (void**)&image);
    if (efi::Error(status))
    {
        METAL_LOG(Error) << u8"Failed to access efi::LoadedImageProtocol: " << (void*)status;
        return mtl::unexpected(status);
    }

    efi::SimpleFileSystemProtocol* fs;
    status = g_efiBootServices->HandleProtocol(image->deviceHandle,
                                               &efi::SimpleFileSystemProtocolGuid, (void**)&fs);
    if (efi::Error(status))
    {
        METAL_LOG(Error) << u8"Failed to access efi::LoadedImageProtocol: " << (void*)status;
        return mtl::unexpected(status);
    }

    efi::FileProtocol* volume;
    status = fs->OpenVolume(fs, &volume);
    if (efi::Error(status))
    {
        METAL_LOG(Error) << u8"Failed to open file system volume: " << (void*)status;
        return mtl::unexpected(status);
    }

    efi::FileProtocol* directory;
    status = volume->Open(volume, &directory, L"\\EFI\\rainbow", efi::FileModeRead, 0);
    if (efi::Error(status))
    {
        METAL_LOG(Error) << u8"Failed to open Rainbow directory: " << (void*)status;
        return mtl::unexpected(status);
    }

    return directory;
}

mtl::expected<Module, efi::Status> LoadModule(std::string_view name)
{
    assert(g_fileSystem);

    // Technically we should be doing "proper" conversion to a wchar_t string here,
    // but we know that "name" will always be valid ASCII. So we take a shortcut.
    // We also don't have a std::wstring implementation, so we use a std::Vector.
    std::vector<wchar_t> pathBuffer(name.begin(), name.end());
    pathBuffer.push_back('\0');
    const auto path = pathBuffer.data();

    efi::FileProtocol* file;
    auto status = g_fileSystem->Open(g_fileSystem, &file, path, efi::FileModeRead, 0);
    if (efi::Error(status))
    {
        METAL_LOG(Debug) << u8"Failed to open file \"" << path << u8"\": " << (void*)status;
        return mtl::unexpected(status);
    }

    std::vector<char> infoBuffer;
    efi::uintn_t infoSize = 0;
    while ((status = file->GetInfo(file, &efi::FileInfoGuid, &infoSize, infoBuffer.data())) ==
           efi::BufferTooSmall)
    {
        infoBuffer.resize(infoSize);
    }
    if (efi::Error(status))
    {
        METAL_LOG(Debug) << u8"Failed to retrieve info about file \"" << path << u8"\": "
                         << (void*)status;
        ;
        return mtl::unexpected(status);
    }

    const efi::FileInfo& info = *(const efi::FileInfo*)infoBuffer.data();

    // Allocate memory to hold the file
    // We use pages because we want ELF files to be page-aligned
    const int pageCount =
        mtl::align_up(info.fileSize, mtl::MEMORY_PAGE_SIZE) >> mtl::MEMORY_PAGE_SHIFT;
    efi::PhysicalAddress fileAddress;
    status = g_efiBootServices->AllocatePages(efi::AllocateAnyPages, efi::EfiLoaderData, pageCount,
                                              &fileAddress);
    if (efi::Error(status))
    {
        METAL_LOG(Debug) << u8"Failed to allocate memory (" << pageCount << u8" pages) for file \""
                         << path << u8"\": " << (void*)status;
        return mtl::unexpected(status);
    }

    void* data = (void*)(uintptr_t)fileAddress;
    efi::uintn_t fileSize = info.fileSize;
    status = file->Read(file, &fileSize, data);
    if (efi::Error(status))
    {
        METAL_LOG(Debug) << u8"Failed to load file \"" << path << u8"\": " << (void*)status;
        return mtl::unexpected(status);
    }

    return Module{fileAddress, fileSize};
}

// TODO: we'd like to return a smart pointer here, don't we?
mtl::expected<MemoryMap*, efi::Status> ExitBootServices()
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
        return mtl::unexpected(status);
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
            return mtl::unexpected(status);
        }
    }

    if (efi::Error(status))
    {
        METAL_LOG(Fatal) << u8"Failed to exit boot services: " << (void*)status;
        return mtl::unexpected(status);
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

    // Remove loggers that aren't usable anymore
    mtl::g_log.RemoveLogger(s_efiLogger);
    s_efiLogger = nullptr;

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
    mtl::g_log.AddLogger(&console);
    s_efiLogger = &console;

    METAL_LOG(Info) << u8"UEFI firmware vendor: " << g_efiSystemTable->firmwareVendor;
    METAL_LOG(Info) << u8"UEFI firmware revision: " << (g_efiSystemTable->firmwareRevision >> 16)
                    << u8'.' << (g_efiSystemTable->firmwareRevision & 0xFFFF);

    if (auto directory = OpenRainbowDirectory())
    {
        g_fileSystem = *directory;
    }
    else
    {
        METAL_LOG(Fatal) << u8"Unable to access file system";
        return directory.error();
    }

    return Boot();
}
