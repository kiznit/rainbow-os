/*
    Copyright (c) 2018, Thierry Tremblay
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
#include <uefi.h>
#include <Guid/FileInfo.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include "eficonsole.hpp"
#include "log.hpp"
#include "x86.hpp"


EFI_HANDLE g_efiImage;
EFI_SYSTEM_TABLE* ST;
EFI_BOOT_SERVICES* BS;

static EFI_GUID g_efiFileInfoGuid = EFI_FILE_INFO_ID;
static EFI_GUID g_efiLoadedImageProtocolGuid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
static EFI_GUID g_efiSimpleFileSystemProtocolGuid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;


void InitDisplays();



static void* AllocatePages(size_t pageCount, physaddr_t maxAddress)
{
    EFI_PHYSICAL_ADDRESS memory = maxAddress - 1;
    auto status = BS->AllocatePages(AllocateMaxAddress, EfiLoaderData, pageCount, &memory);
    if (EFI_ERROR(status))
    {
        return nullptr;
    }

    return (void*)memory;
}


// Look at this code and tell me EFI isn't insane
static EFI_STATUS LoadKernel(const wchar_t* path)
{
    EFI_LOADED_IMAGE_PROTOCOL* image = nullptr;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs = nullptr;
    EFI_FILE_PROTOCOL* fileSystemRoot = nullptr;
    EFI_FILE_PROTOCOL* file = nullptr;
    EFI_FILE_INFO* info = nullptr;
    void* kernel = nullptr;
    UINTN size;
    EFI_STATUS status;

    // Get access to the boot file system
    status = BS->HandleProtocol(g_efiImage, &g_efiLoadedImageProtocolGuid, (void**)&image);
    if (EFI_ERROR(status) || !image)
        goto error;

    status = BS->HandleProtocol(image->DeviceHandle, &g_efiSimpleFileSystemProtocolGuid, (void**)&fs);
    if (EFI_ERROR(status) || !fs)
        goto error;

    // Open the file system
    status = fs->OpenVolume(fs, &fileSystemRoot);
    if (EFI_ERROR(status))
        goto error;

    // Open the kernel file
    status = fileSystemRoot->Open(fileSystemRoot, &file, const_cast<wchar_t*>(path), EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(status))
        goto error;

    // Retrieve the kernel's size
    size = 0;
    status = file->GetInfo(file, &g_efiFileInfoGuid, &size, NULL);
    if (EFI_ERROR(status) && status != EFI_BUFFER_TOO_SMALL)
        goto error;

    status = BS->AllocatePool(EfiLoaderData, size, (void**)&info);
    if (EFI_ERROR(status))
        goto error;

    status = file->GetInfo(file, &g_efiFileInfoGuid, &size, info);
    if (EFI_ERROR(status))
        goto error;

    // Allocate memory to hold the kernel
    status = BS->AllocatePool(EfiLoaderData, info->FileSize, &kernel);
    if (EFI_ERROR(status))
        goto error;

    // Read the kernel into memory
    size = info->FileSize;
    status = file->Read(file, &size, kernel);
    if (EFI_ERROR(status) || size != info->FileSize)
        goto error;


    Log("Kernel loaded at: %p, size: %p\n", kernel, size);

    //todo
    //g_bootInfo.kernelAddress = (uintptr_t)kernel;
    //g_bootInfo.kernelSize = size;

    goto exit;

error:
    if (kernel) BS->FreePool(kernel);

exit:
    if (info) BS->FreePool(info);
    if (file) file->Close(file);
    if (fileSystemRoot) fileSystemRoot->Close(fileSystemRoot);

    return status;
}



extern "C" EFI_STATUS efi_main(EFI_HANDLE hImage, EFI_SYSTEM_TABLE* systemTable)
{
    g_efiImage = hImage;
    ST = systemTable;
    BS = systemTable->BootServices;

    EfiConsole efiConsole;
    efiConsole.Initialize();
    g_console = &efiConsole;

    // It is in theory possible for EFI_BOOT_SERVICES::AllocatePages() to return
    // an allocation at address 0. We do not want this to happen as we use NULL
    // to indicate errors / out-of-memory condition. To ensure it doesn't happen,
    // we attempt to allocate the first memory page. We do not care whether or
    // not it succeeds.
    AllocatePages(1, MEMORY_PAGE_SIZE);

    Log("EFI Bootloader (" STRINGIZE(BOOT_ARCH) ")\n\n");

    Log("Detecting displays...\n");

    InitDisplays();

    auto status = LoadKernel(L"\\EFI\\rainbow\\kernel");
    if (EFI_ERROR(status))
    {
        Log("Failed to load kernel: %p\n", status);
        //todo: return status;
    }

    for(;;);

    return EFI_SUCCESS;
}
