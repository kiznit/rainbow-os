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

#include "EfiConsole.hpp"
#include "EfiDisplay.hpp"
#include "EfiFile.hpp"
#include "VirtualMemory.hpp"
#include "elf.hpp"
#include "uefi.hpp"
#include <expected>
#include <metal/helpers.hpp>
#include <rainbow/uefi/filesystem.hpp>
#include <rainbow/uefi/image.hpp>
#include <string>

static std::vector<mtl::Logger*> s_efiLoggers; // TODO: smart pointers?

static std::expected<char16_t, efi::Status> GetChar(efi::BootServices* bootServices, efi::SimpleTextInputProtocol* conin)
{
    for (;;)
    {
        efi::uintn_t index;
        auto status = bootServices->WaitForEvent(1, &conin->waitForKey, &index);
        if (efi::Error(status))
            return std::unexpected(status);

        efi::InputKey key;
        status = conin->ReadKeyStroke(conin, &key);
        if (efi::Error(status))
        {
            if (status == efi::NotReady)
            {
                continue;
            }

            return std::unexpected(status);
        }

        return key.unicodeChar;
    }
}

static std::expected<efi::FileProtocol*, efi::Status> InitializeFileSystem(efi::BootServices* bootServices, efi::Handle hImage)
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
    status = volume->Open(volume, &directory, u"\\EFI\\rainbow", efi::FileModeRead, 0);
    if (efi::Error(status))
    {
        MTL_LOG(Error) << "Failed to open Rainbow directory: " << mtl::hex(status);
        return std::unexpected(status);
    }

    return directory;
}

std::expected<Module, efi::Status> LoadModule(efi::FileProtocol* fileSystem, std::string_view name)
{
    // Technically we should be doing "proper" conversion to u16string here,
    // but we know that "name" will always be valid ASCII. So we take a shortcut.
    std::u16string path(name.begin(), name.end());

    efi::FileProtocol* file;
    auto status = fileSystem->Open(fileSystem, &file, path.c_str(), efi::FileModeRead, 0);
    if (efi::Error(status))
    {
        MTL_LOG(Debug) << "Failed to open file \"" << path << "\": " << mtl::hex(status);
        return std::unexpected(status);
    }

    std::vector<char> infoBuffer;
    efi::uintn_t infoSize = 0;
    while ((status = file->GetInfo(file, &efi::FileInfoGuid, &infoSize, infoBuffer.data())) == efi::BufferTooSmall)
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

static void PrintBanner(efi::SimpleTextOutputProtocol* conout)
{
    conout->SetAttribute(conout, efi::BackgroundBlack);
    conout->ClearScreen(conout);

    conout->SetAttribute(conout, efi::Red);
    conout->OutputString(conout, u"R");
    conout->SetAttribute(conout, efi::LightRed);
    conout->OutputString(conout, u"a");
    conout->SetAttribute(conout, efi::Yellow);
    conout->OutputString(conout, u"i");
    conout->SetAttribute(conout, efi::LightGreen);
    conout->OutputString(conout, u"n");
    conout->SetAttribute(conout, efi::LightCyan);
    conout->OutputString(conout, u"b");
    conout->SetAttribute(conout, efi::LightBlue);
    conout->OutputString(conout, u"o");
    conout->SetAttribute(conout, efi::LightMagenta);
    conout->OutputString(conout, u"w");
    conout->SetAttribute(conout, efi::LightGray);

    conout->OutputString(conout, u" UEFI bootloader\n\r\n\r");
}

static void SetupConsoleLogging(efi::SimpleTextOutputProtocol* conout)
{
    const auto console = new EfiConsole(conout);
    mtl::g_log.AddLogger(console);
    s_efiLoggers.push_back(console);
}

static std::expected<void, efi::Status> SetupFileLogging(efi::FileProtocol* fileSystem)
{
    assert(fileSystem);

    efi::FileProtocol* file;
    auto status = fileSystem->Open(fileSystem, &file, u"boot.log", efi::FileModeCreate, 0);
    if (efi::Error(status))
    {
        return std::unexpected(status);
    }

    const auto logfile = new EfiFile(file);
    mtl::g_log.AddLogger(logfile);
    s_efiLoggers.push_back(logfile);

    return {};
}

efi::Status Boot(efi::Handle hImage, efi::SystemTable* systemTable)
{
    auto fileSystem = InitializeFileSystem(systemTable->bootServices, hImage);
    if (!fileSystem)
    {
        MTL_LOG(Fatal) << "Unable to access file system: " << mtl::hex(fileSystem.error());
        return fileSystem.error();
    }

    auto status = SetupFileLogging(*fileSystem);
    if (!status)
    {
        MTL_LOG(Warning) << "Failed to create log file: " << mtl::hex(status.error());
    }

    MTL_LOG(Info) << "System architecture: " << MTL_STRINGIZE(ARCH);
    MTL_LOG(Info) << "UEFI firmware vendor: " << systemTable->firmwareVendor;
    MTL_LOG(Info) << "UEFI firmware revision: " << (systemTable->firmwareRevision >> 16) << "."
                  << (systemTable->firmwareRevision & 0xFFFF);

    auto kernel = LoadModule(*fileSystem, "kernel");
    if (!kernel)
    {
        MTL_LOG(Fatal) << "Failed to load kernel image: " << mtl::hex(kernel.error());
        return kernel.error();
    }

    MTL_LOG(Info) << "Kernel size: " << kernel->size << " bytes";

    VirtualMemory vmm;
    if (!elf_load(*kernel, vmm))
    {
        MTL_LOG(Fatal) << "Failed to load kernel module";
        return efi::LoadError;
    }

    auto displays = InitializeDisplays(systemTable->bootServices);

    auto memoryMap = ExitBootServices();
    if (!memoryMap)
    {
        return memoryMap.error();
    }

    // Once we have exited boot services, we can never return
    for (;;)
        ;
}

efi::Status efi_main(efi::Handle hImage, efi::SystemTable* systemTable)
{
    const auto conout = systemTable->conOut;

    PrintBanner(conout);

    SetupConsoleLogging(conout);

    auto status = Boot(hImage, systemTable);

    MTL_LOG(Fatal) << "Failed to boot: " << mtl::hex(status);

    conout->OutputString(conout, u"<Press any key to exit>");
    GetChar(systemTable->bootServices, systemTable->conIn);

    return status;
}
