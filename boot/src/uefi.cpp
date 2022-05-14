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

#include "uefi.hpp"
#include "EfiConsole.hpp"
#include "EfiFile.hpp"
#include <cassert>
#include <metal/log.hpp>
#include <rainbow/uefi/image.hpp>

efi::Handle g_efiImage;
efi::SystemTable* g_efiSystemTable;
efi::BootServices* g_efiBootServices;
efi::RuntimeServices* g_efiRuntimeServices;

static std::shared_ptr<MemoryMap> g_memoryMap;
static std::vector<std::unique_ptr<mtl::Logger>> s_efiLoggers;

std::expected<mtl::PhysicalAddress, efi::Status> AllocatePages(size_t pageCount)
{
    if (g_efiBootServices)
    {
        efi::PhysicalAddress memory{0};
        const auto status =
            g_efiBootServices->AllocatePages(efi::AllocateType::AnyPages, efi::MemoryType::LoaderData, pageCount, &memory);

        if (!efi::Error(status))
            return memory;
    }

    if (g_memoryMap)
    {
        const auto memory = g_memoryMap->AllocatePages(pageCount);
        if (memory)
            return *memory;
    }

    return std::unexpected(efi::Status::OutOfResource);
}

std::expected<std::shared_ptr<MemoryMap>, efi::Status> ExitBootServices()
{
    efi::uintn_t bufferSize = 0;
    efi::MemoryDescriptor* descriptors = nullptr;
    efi::uintn_t memoryMapKey = 0;
    efi::uintn_t descriptorSize = 0;
    uint32_t descriptorVersion = 0;
    std::vector<efi::MemoryDescriptor> memoryMap;

    // 1) Retrieve the memory map from the firmware
    efi::Status status;
    std::vector<char> buffer;
    while ((status = g_efiBootServices->GetMemoryMap(&bufferSize, descriptors, &memoryMapKey, &descriptorSize,
                                                     &descriptorVersion)) == efi::Status::BufferTooSmall)
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
        MTL_LOG(Fatal) << "Failed to retrieve the EFI memory map (1): " << mtl::hex(status);
        return std::unexpected(status);
    }

    // 2) Exit boot services - it is possible for the firmware to modify the memory map
    // during a call to ExitBootServices(). A so-called "partial shutdown".
    // When that happens, ExitBootServices() will return EFI_INVALID_PARAMETER.
    while ((status = g_efiBootServices->ExitBootServices(g_efiImage, memoryMapKey)) == efi::Status::InvalidParameter)
    {
        // Memory map changed during ExitBootServices(), the only APIs we are allowed to
        // call at this point are GetMemoryMap() and ExitBootServices().
        bufferSize = buffer.size();
        status = g_efiBootServices->GetMemoryMap(&bufferSize, descriptors, &memoryMapKey, &descriptorSize, &descriptorVersion);
        if (efi::Error(status))
        {
            MTL_LOG(Fatal) << "Failed to retrieve the EFI memory map (2): " << mtl::hex(status);
            return std::unexpected(status);
        }
    }

    if (efi::Error(status))
    {
        MTL_LOG(Fatal) << "Failed to exit boot services: " << mtl::hex(status);
        return std::unexpected(status);
    }

    // Note we can't allocate memory until g_memoryMap is set

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
    for (auto& logger : s_efiLoggers)
    {
        mtl::g_log.RemoveLogger(logger.get());
    }

    // Build the memory map (descriptors might be bigger than sizeof(efi::MemoryDescriptor), so we need to copy them).
    auto descriptor = descriptors;
    const auto descriptorCount = bufferSize / descriptorSize;
    for (efi::uintn_t i = 0; i != descriptorCount;
         ++i, descriptor = (efi::MemoryDescriptor*)((uintptr_t)descriptor + descriptorSize))
    {
        memoryMap.emplace_back(*descriptor);
    }
    g_memoryMap = std::make_shared<MemoryMap>(std::move(memoryMap));

    return g_memoryMap;
}

std::expected<char16_t, efi::Status> GetChar()
{
    auto conin = g_efiSystemTable->conIn;

    for (;;)
    {
        efi::uintn_t index;
        auto status = g_efiBootServices->WaitForEvent(1, &conin->waitForKey, &index);
        if (efi::Error(status))
            return std::unexpected(status);

        efi::InputKey key;
        status = conin->ReadKeyStroke(conin, &key);
        if (efi::Error(status))
        {
            if (status == efi::Status::NotReady)
            {
                continue;
            }

            return std::unexpected(status);
        }

        return key.unicodeChar;
    }
}

std::vector<EfiDisplay> InitializeDisplays(efi::BootServices* bootServices)
{
    std::vector<EfiDisplay> displays;

    efi::uintn_t size{0};
    std::vector<efi::Handle> handles;
    efi::Status status;

    // LocateHandle() should only be called twice... But I don't want to write it twice :)
    while ((status = bootServices->LocateHandle(efi::LocateSearchType::ByProtocol, &efi::GraphicsOutputProtocolGuid, nullptr, &size,
                                                handles.data())) == efi::Status::BufferTooSmall)
    {
        handles.resize(size / sizeof(efi::Handle));
    }

    if (efi::Error(status))
    {
        // Likely efi::NotFound, but any error should be handled as "no display available"
        MTL_LOG(Warning) << "Not UEFI displays found: " << mtl::hex(status);
        return displays;
    }

    for (auto handle : handles)
    {
        efi::DevicePathProtocol* dpp = nullptr;
        if (efi::Error(bootServices->HandleProtocol(handle, &efi::DevicePathProtocolGuid, (void**)&dpp)))
            continue;

        // If dpp is null, this is the "Console Splitter" driver. It is used to draw on all
        // screens at the same time and doesn't represent a real hardware device.
        if (!dpp)
            continue;

        efi::GraphicsOutputProtocol* gop{nullptr};
        if (efi::Error(bootServices->HandleProtocol(handle, &efi::GraphicsOutputProtocolGuid, (void**)&gop)))
            continue;
        // gop is not expected to be null, but let's play safe.
        if (!gop)
            continue;

        efi::EdidProtocol* edid{nullptr};
        if (efi::Error(bootServices->HandleProtocol(handle, &efi::EdidActiveProtocolGuid, (void**)&edid)) || (!edid))
        {
            if (efi::Error(bootServices->HandleProtocol(handle, &efi::EdidDiscoveredProtocolGuid, (void**)&edid)))
                edid = nullptr;
        }

        const auto& mode = *gop->mode->info;
        MTL_LOG(Info) << "Display: " << mode.horizontalResolution << " x " << mode.verticalResolution
                      << ", edid size: " << (edid ? edid->sizeOfEdid : 0) << " bytes";

        displays.emplace_back(gop, edid);
    }

    return displays;
}

std::expected<efi::FileProtocol*, efi::Status> InitializeFileSystem()
{
    efi::Status status;

    efi::LoadedImageProtocol* image;
    status = g_efiBootServices->HandleProtocol(g_efiImage, &efi::LoadedImageProtocolGuid, (void**)&image);
    if (efi::Error(status))
    {
        MTL_LOG(Error) << "Failed to access efi::LoadedImageProtocol: " << mtl::hex(status);
        return std::unexpected(status);
    }

    efi::SimpleFileSystemProtocol* fs;
    status = g_efiBootServices->HandleProtocol(image->deviceHandle, &efi::SimpleFileSystemProtocolGuid, (void**)&fs);
    if (efi::Error(status))
    {
        MTL_LOG(Error) << "Failed to access efi::LoadedImageProtocol: " << mtl::hex(status);
        return std::unexpected(status);
    }

    efi::FileProtocol* volume;
    status = fs->OpenVolume(fs, &volume);
    if (efi::Error(status))
    {
        MTL_LOG(Error) << "Failed to open file system volume: " << mtl::hex(status);
        return std::unexpected(status);
    }

    efi::FileProtocol* directory;
    status = volume->Open(volume, &directory, u"\\EFI\\rainbow", efi::OpenMode::Read, 0);
    if (efi::Error(status))
    {
        MTL_LOG(Error) << "Failed to open Rainbow directory: " << mtl::hex(status);
        return std::unexpected(status);
    }

    return directory;
}

void SetupConsoleLogging()
{
    auto console = std::make_unique<EfiConsole>(g_efiSystemTable->conOut);
    mtl::g_log.AddLogger(console.get());
    s_efiLoggers.emplace_back(std::move(console));
}

std::expected<void, efi::Status> SetupFileLogging(efi::FileProtocol* fileSystem)
{
    assert(fileSystem);

    efi::FileProtocol* file;
    auto status = fileSystem->Open(fileSystem, &file, u"boot.log", efi::OpenMode::Create, 0);
    if (efi::Error(status))
    {
        return std::unexpected(status);
    }

    auto logfile = std::make_unique<EfiFile>(file);
    logfile->Write(u8"Rainbow UEFI bootloader\n\n"); // TODO: this is not great
    mtl::g_log.AddLogger(logfile.get());
    s_efiLoggers.emplace_back(std::move(logfile));

    return {};
}
