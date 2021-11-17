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
#include "EfiFile.hpp"
#include "MemoryMap.hpp"
#include <metal/helpers.hpp>
#include <metal/log.hpp>
#include <rainbow/uefi/edid.hpp>
#include <rainbow/uefi/filesystem.hpp>
#include <rainbow/uefi/graphics.hpp>
#include <rainbow/uefi/image.hpp>
#include <string>
#include <vector>

efi::Handle g_efiImage;
efi::SystemTable* g_efiSystemTable;
efi::BootServices* g_efiBootServices;
efi::RuntimeServices* g_efiRuntimeServices;

static std::vector<mtl::Logger*> s_efiLoggers;
static efi::FileProtocol* g_fileSystem;

void InitializeDisplays()
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
        MTL_LOG(Warning) << "Not UEFI displays found: " << mtl::hex(status);
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
        MTL_LOG(Info) << "Display: " << mode.horizontalResolution << " x "
                      << mode.verticalResolution << ", edid size: " << (edid ? edid->sizeOfEdid : 0)
                      << " bytes";
    }
}

static mtl::expected<void, efi::Status> InitializeFileSystem()
{
    efi::Status status;

    efi::LoadedImageProtocol* image;
    status = g_efiBootServices->HandleProtocol(g_efiImage, &efi::LoadedImageProtocolGuid,
                                               (void**)&image);
    if (efi::Error(status))
    {
        MTL_LOG(Error) << "Failed to access efi::LoadedImageProtocol: " << mtl::hex(status);
        return mtl::unexpected(status);
    }

    efi::SimpleFileSystemProtocol* fs;
    status = g_efiBootServices->HandleProtocol(image->deviceHandle,
                                               &efi::SimpleFileSystemProtocolGuid, (void**)&fs);
    if (efi::Error(status))
    {
        MTL_LOG(Error) << "Failed to access efi::LoadedImageProtocol: " << mtl::hex(status);
        return mtl::unexpected(status);
    }

    efi::FileProtocol* volume;
    status = fs->OpenVolume(fs, &volume);
    if (efi::Error(status))
    {
        MTL_LOG(Error) << "Failed to open file system volume: " << mtl::hex(status);
        return mtl::unexpected(status);
    }

    efi::FileProtocol* directory;
    status = volume->Open(volume, &directory, u"\\EFI\\rainbow", efi::FileModeRead, 0);
    if (efi::Error(status))
    {
        MTL_LOG(Error) << "Failed to open Rainbow directory: " << mtl::hex(status);
        return mtl::unexpected(status);
    }

    g_fileSystem = directory;
}

mtl::expected<Module, efi::Status> LoadModule(std::string_view name)
{
    assert(g_fileSystem);

    // Technically we should be doing "proper" conversion to u16string here,
    // but we know that "name" will always be valid ASCII. So we take a shortcut.
    std::u16string path(name.begin(), name.end());

    efi::FileProtocol* file;
    auto status = g_fileSystem->Open(g_fileSystem, &file, path.c_str(), efi::FileModeRead, 0);
    if (efi::Error(status))
    {
        MTL_LOG(Debug) << "Failed to open file \"" << path << "\": " << mtl::hex(status);
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
        MTL_LOG(Debug) << "Failed to retrieve info about file \"" << path
                       << "\": " << mtl::hex(status);
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
        MTL_LOG(Debug) << "Failed to allocate memory (" << pageCount << " pages) for file \""
                       << path << "\": " << mtl::hex(status);
        return mtl::unexpected(status);
    }

    void* data = (void*)(uintptr_t)fileAddress;
    efi::uintn_t fileSize = info.fileSize;
    status = file->Read(file, &fileSize, data);
    if (efi::Error(status))
    {
        MTL_LOG(Debug) << "Failed to load file \"" << path << "\": " << mtl::hex(status);
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
        MTL_LOG(Fatal) << "Failed to retrieve the EFI memory map (1): " << mtl::hex(status);
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
            MTL_LOG(Fatal) << "Failed to retrieve the EFI memory map (2): " << mtl::hex(status);
            return mtl::unexpected(status);
        }
    }

    if (efi::Error(status))
    {
        MTL_LOG(Fatal) << "Failed to exit boot services: " << mtl::hex(status);
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
    for (auto logger : s_efiLoggers)
    {
        mtl::g_log.RemoveLogger(logger);
    }

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
    console->OutputString(console, u"R");
    console->SetAttribute(console, efi::LightRed);
    console->OutputString(console, u"a");
    console->SetAttribute(console, efi::Yellow);
    console->OutputString(console, u"i");
    console->SetAttribute(console, efi::LightGreen);
    console->OutputString(console, u"n");
    console->SetAttribute(console, efi::LightCyan);
    console->OutputString(console, u"b");
    console->SetAttribute(console, efi::LightBlue);
    console->OutputString(console, u"o");
    console->SetAttribute(console, efi::LightMagenta);
    console->OutputString(console, u"w");
    console->SetAttribute(console, efi::LightGray);

    console->OutputString(console, u" UEFI bootloader\n\r\n\r");
}

static void SetupConsoleLogging()
{
    const auto console = new EfiConsole(g_efiSystemTable->conOut);
    mtl::g_log.AddLogger(console);
    s_efiLoggers.push_back(console);
}

static efi::Status SetupFileLogging()
{
    assert(g_fileSystem);

    efi::FileProtocol* file;
    auto status = g_fileSystem->Open(g_fileSystem, &file, u"boot.log", efi::FileModeCreate, 0);
    if (efi::Error(status))
    {
        return status;
    }

    const auto logfile = new EfiFile(file);
    mtl::g_log.AddLogger(logfile);
    s_efiLoggers.push_back(logfile);

    return efi::Success;
}

// Cannot use "main()" as the function name as this causes problems with mingw
efi::Status efi_main()
{
    PrintBanner(g_efiSystemTable->conOut);

    SetupConsoleLogging();

    auto status = InitializeFileSystem();
    if (efi::Error(status))
    {
        MTL_LOG(Fatal) << "Unable to access file system: " << mtl::hex(status);
        // TODO: instead of returning the status, we should wait for a key press and then reboot the
        // machine (?)
        return status;
    }

    status = SetupFileLogging();
    if (efi::Error(status))
    {
        MTL_LOG(Warning) << "Failed to create log file: " << mtl::hex(status);
    }

    MTL_LOG(Info) << "UEFI firmware vendor: " << g_efiSystemTable->firmwareVendor;
    MTL_LOG(Info) << "UEFI firmware revision: " << (g_efiSystemTable->firmwareRevision >> 16) << "."
                  << (g_efiSystemTable->firmwareRevision & 0xFFFF);

    return Boot();
}
