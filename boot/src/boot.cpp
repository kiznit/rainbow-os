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

#include "boot.hpp"
#include "Console.hpp"
#include "GraphicsDisplay.hpp"
#include "LogFile.hpp"
#include "PageTable.hpp"
#include "acpi.hpp"
#include "elf.hpp"
#include <cassert>
#include <metal/graphics/GraphicsConsole.hpp>
#include <metal/graphics/SimpleDisplay.hpp>
#include <metal/helpers.hpp>
#include <rainbow/acpi.hpp>
#include <rainbow/boot.hpp>
#include <rainbow/pci.hpp>
#include <rainbow/uefi/filesystem.hpp>
#include <rainbow/uefi/image.hpp>
#include <string>

bool CheckArch();
[[noreturn]] void JumpToKernel(const BootInfo& bootInfo, const void* kernelEntryPoint, PageTable& pageTable);

extern efi::SystemTable* g_efiSystemTable; // TODO: this is only needed for AllocatePages, and that sucks.

static std::shared_ptr<MemoryMap> g_memoryMap;
static std::vector<std::shared_ptr<mtl::Logger>> g_efiLoggers;

std::expected<mtl::PhysicalAddress, efi::Status> AllocatePages(size_t pageCount)
{
    auto bootServices = g_efiSystemTable->bootServices;

    if (bootServices)
    {
        efi::PhysicalAddress memory{0};
        const auto status =
            bootServices->AllocatePages(efi::AllocateType::AnyPages, efi::MemoryType::LoaderData, pageCount, &memory);

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

std::shared_ptr<Console> InitializeConsole(efi::SystemTable* systemTable)
{
    auto conout = systemTable->conout;
    conout->SetAttribute(conout, efi::TextAttribute::LightGray | efi::TextAttribute::BackgroundBlack);
    conout->ClearScreen(conout);
    conout->SetAttribute(conout, efi::TextAttribute::Red);
    conout->OutputString(conout, u"R");
    conout->SetAttribute(conout, efi::TextAttribute::LightRed);
    conout->OutputString(conout, u"a");
    conout->SetAttribute(conout, efi::TextAttribute::Yellow);
    conout->OutputString(conout, u"i");
    conout->SetAttribute(conout, efi::TextAttribute::LightGreen);
    conout->OutputString(conout, u"n");
    conout->SetAttribute(conout, efi::TextAttribute::LightCyan);
    conout->OutputString(conout, u"b");
    conout->SetAttribute(conout, efi::TextAttribute::LightBlue);
    conout->OutputString(conout, u"o");
    conout->SetAttribute(conout, efi::TextAttribute::LightMagenta);
    conout->OutputString(conout, u"w");
    conout->SetAttribute(conout, efi::TextAttribute::LightGray);
    conout->OutputString(conout, u" UEFI bootloader\n\r\n\r");

    auto console = std::make_shared<Console>(systemTable);
    g_efiLoggers.emplace_back(console);
    mtl::g_log.AddLogger(console.get());

    return console;
}

std::vector<GraphicsDisplay> InitializeDisplays(efi::BootServices* bootServices)
{
    std::vector<GraphicsDisplay> displays;

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

        // TODO: Set best resolution based on EDID and supported resolutions

        const auto& mode = *gop->mode->info;
        MTL_LOG(Info) << "Display: " << mode.horizontalResolution << " x " << mode.verticalResolution
                      << ", edid size: " << (edid ? edid->sizeOfEdid : 0) << " bytes";

        displays.emplace_back(gop, edid);
    }

    return displays;
}

std::expected<efi::FileProtocol*, efi::Status> InitializeFileSystem(efi::Handle hImage, efi::BootServices* bootServices)
{
    efi::Status status;

    efi::LoadedImageProtocol* image;
    status = bootServices->HandleProtocol(hImage, &efi::LoadedImageProtocolGuid, (void**)&image);
    if (efi::Error(status))
    {
        MTL_LOG(Error) << "Failed to access efi::LoadedImageProtocol: " << mtl::hex(status);
        return std::unexpected(status);
    }

    efi::SimpleFileSystemProtocol* fs;
    status = bootServices->HandleProtocol(image->deviceHandle, &efi::SimpleFileSystemProtocolGuid, (void**)&fs);
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

std::expected<std::shared_ptr<LogFile>, efi::Status> InitializeLogFile(efi::FileProtocol* fileSystem)
{
    efi::FileProtocol* file;
    auto status = fileSystem->Open(fileSystem, &file, u"boot.log", efi::OpenMode::Create, 0);
    if (efi::Error(status))
        return std::unexpected(status);

    auto logFile = std::make_shared<LogFile>(file);
    g_efiLoggers.emplace_back(logFile);
    mtl::g_log.AddLogger(logFile.get());

    logFile->Write(u8"Rainbow UEFI bootloader\n\n");

    return logFile;
}

const acpi::Rsdp* FindAcpiRsdp(const efi::SystemTable* systemTable)
{
    const acpi::Rsdp* result = nullptr;

    for (unsigned int i = 0; i != systemTable->numberOfTableEntries; ++i)
    {
        const auto& table = systemTable->configurationTable[i];

        // ACPI 1.0
        if (table.vendorGuid == efi::Acpi1TableGuid)
        {
            const auto rsdp = (acpi::Rsdp*)table.vendorTable;
            if (rsdp && rsdp->VerifyChecksum())
            {
                result = rsdp;
            }
            // Continue looking for ACPI 2.0 table
        }

        // ACPI 2.0
        if (table.vendorGuid == efi::Acpi2TableGuid)
        {
            const auto rsdp = (acpi::RsdpExtended*)table.vendorTable;
            if (rsdp && rsdp->VerifyExtendedChecksum())
            {
                result = rsdp;
                break;
            }
        }
    }

    return result;
}

std::expected<Module, efi::Status> LoadModule(efi::FileProtocol* fileSystem, std::string_view name)
{
    // Technically we should be doing "proper" conversion to u16string here,
    // but we know that "name" will always be valid ASCII. So we take a shortcut.
    std::u16string path(name.begin(), name.end());

    efi::FileProtocol* file;
    auto status = fileSystem->Open(fileSystem, &file, path.c_str(), efi::OpenMode::Read, 0);
    if (efi::Error(status))
    {
        MTL_LOG(Debug) << "Failed to open file \"" << path << "\": " << mtl::hex(status);
        return std::unexpected(status);
    }

    std::vector<char> infoBuffer;
    efi::uintn_t infoSize = 0;
    while ((status = file->GetInfo(file, &efi::FileInfoGuid, &infoSize, infoBuffer.data())) == efi::Status::BufferTooSmall)
    {
        infoBuffer.resize(infoSize);
    }
    if (efi::Error(status))
    {
        MTL_LOG(Debug) << "Failed to retrieve info about file \"" << path << "\": " << mtl::hex(status);
        return std::unexpected(status);
    }

    const efi::FileInfo& info = *(const efi::FileInfo*)infoBuffer.data();

    // Allocate memory to hold the file
    // We use pages because we want ELF files to be page-aligned
    const int pageCount = mtl::align_up(info.fileSize, mtl::MemoryPageSize) >> mtl::MemoryPageShift;
    auto fileAddress = AllocatePages(pageCount);
    if (!fileAddress)
    {
        MTL_LOG(Debug) << "Failed to allocate memory (" << pageCount << " pages) for file \"" << path << "\": " << mtl::hex(status);
        return std::unexpected(fileAddress.error());
    }

    void* data = (void*)(uintptr_t)*fileAddress;
    efi::uintn_t fileSize = info.fileSize;
    status = file->Read(file, &fileSize, data);
    if (efi::Error(status))
    {
        MTL_LOG(Debug) << "Failed to load file \"" << path << "\": " << mtl::hex(status);
        return std::unexpected(status);
    }

    return Module{*fileAddress, fileSize};
}

std::expected<std::shared_ptr<MemoryMap>, efi::Status> ExitBootServices(efi::Handle hImage, efi::SystemTable* systemTable)
{
    efi::uintn_t bufferSize = 0;
    efi::MemoryDescriptor* descriptors = nullptr;
    efi::uintn_t memoryMapKey = 0;
    efi::uintn_t descriptorSize = 0;
    uint32_t descriptorVersion = 0;
    std::vector<efi::MemoryDescriptor> memoryMap;

    auto bootServices = systemTable->bootServices;

    // 1) Retrieve the memory map from the firmware
    efi::Status status;
    std::vector<char> buffer;
    while ((status = bootServices->GetMemoryMap(&bufferSize, descriptors, &memoryMapKey, &descriptorSize, &descriptorVersion)) ==
           efi::Status::BufferTooSmall)
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
    while ((status = bootServices->ExitBootServices(hImage, memoryMapKey)) == efi::Status::InvalidParameter)
    {
        // Memory map changed during ExitBootServices(), the only APIs we are allowed to
        // call at this point are GetMemoryMap() and ExitBootServices().
        bufferSize = buffer.size();
        status = bootServices->GetMemoryMap(&bufferSize, descriptors, &memoryMapKey, &descriptorSize, &descriptorVersion);
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
    systemTable->consoleInHandle = 0;
    systemTable->conin = nullptr;
    systemTable->consoleOutHandle = 0;
    systemTable->conout = nullptr;
    systemTable->standardErrorHandle = 0;
    systemTable->stderr = nullptr;
    systemTable->bootServices = nullptr;

    // Remove loggers that aren't usable anymore
    for (auto& logger : g_efiLoggers)
    {
        mtl::g_log.RemoveLogger(logger.get());
    }
    g_efiLoggers.clear();

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

efi::Status Boot(efi::Handle hImage, efi::SystemTable* systemTable)
{
    auto fileSystem = InitializeFileSystem(hImage, systemTable->bootServices);
    if (!fileSystem)
    {
        MTL_LOG(Fatal) << "Unable to access file system: " << mtl::hex(fileSystem.error());
        return fileSystem.error();
    }

    auto logFile = InitializeLogFile(*fileSystem);
    if (!logFile)
        MTL_LOG(Warning) << "Unable to create log file: " << mtl::hex(logFile.error());

    MTL_LOG(Info) << "System architecture: " << MTL_STRINGIZE(ARCH);
    MTL_LOG(Info) << "UEFI firmware vendor: " << systemTable->firmwareVendor;
    MTL_LOG(Info) << "UEFI firmware revision: " << (systemTable->firmwareRevision >> 16) << "."
                  << (systemTable->firmwareRevision & 0xFFFF);

    if (!CheckArch())
    {
        MTL_LOG(Fatal) << "Requirements for Rainbow OS not met";
        return efi::Status::Unsupported;
    }

    const auto rsdp = FindAcpiRsdp(systemTable);
    if (!rsdp)
    {
        MTL_LOG(Fatal) << "ACPI not found";
        return efi::Status::Unsupported;
    }

    MTL_LOG(Info) << "Found ACPI " << (rsdp->revision ? rsdp->revision : 1) << ", RSDP at " << rsdp;

    EnumerateTables(rsdp);

    if (rsdp->revision < 2)
    {
        MTL_LOG(Fatal) << "ACPI 2 or above is required";
        return efi::Status::Unsupported;
    }

    auto kernel = LoadModule(*fileSystem, "kernel");
    if (!kernel)
    {
        MTL_LOG(Fatal) << "Failed to load kernel image: " << mtl::hex(kernel.error());
        return kernel.error();
    }
    MTL_LOG(Info) << "Kernel size: " << kernel->size << " bytes";

    PageTable pageTable;
    auto kernelEntryPoint = elf_load(*kernel, pageTable);
    if (!kernelEntryPoint)
    {
        MTL_LOG(Fatal) << "Failed to load kernel module";
        return efi::Status::LoadError;
    }

    // TODO: pass framebuffer information to the kernel
    auto displays = InitializeDisplays(systemTable->bootServices);
    // TODO: sort out ownership issues
    std::unique_ptr<mtl::SimpleDisplay> display;
    std::unique_ptr<mtl::GraphicsConsole> console;
    if (!displays.empty() && displays[0].GetFrontbuffer())
    {
        display.reset(new mtl::SimpleDisplay(displays[0].GetFrontbuffer(), displays[0].GetBackbuffer()));
        console.reset(new mtl::GraphicsConsole(display.get()));
    }

#if defined(__x86_64__)
    pci::EnumerateDevices();
#endif
    for (;;)
        ;

    MTL_LOG(Info) << "Exiting boot services...";
    auto memoryMap = ExitBootServices(hImage, systemTable);
    if (!memoryMap)
        return memoryMap.error();

    if (console)
    {
        console->Clear();
        mtl::g_log.AddLogger(console.get());
    }

    BootInfo bootInfo{.version = RAINBOW_BOOT_VERSION,
                      .memoryMapLength = (uint32_t)(*memoryMap)->size(),
                      .memoryMap = (uintptr_t)(*memoryMap)->data(),
                      .acpiRsdp = (uintptr_t)rsdp};

    MTL_LOG(Info) << "Jumping to kernel...";
    JumpToKernel(bootInfo, kernelEntryPoint, pageTable);

    // Not reachable
    assert(0);
}

efi::Status efi_main(efi::Handle hImage, efi::SystemTable* systemTable)
{
    auto console = InitializeConsole(systemTable);

    auto status = Boot(hImage, systemTable);

    console->Write(u"\n<Press any key to exit>");
    console->GetChar();

    return status;
}
