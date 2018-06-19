/*
    Copyright (c) 2016, Thierry Tremblay
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

#include <stdio.h>
#include <stdlib.h>
#include <Uefi.h>
#include <Guid/FileInfo.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>

#include "boot.hpp"
#include "eficonsole.hpp"
#include "memory.hpp"
#include "graphics/graphicsconsole.hpp"
#include "graphics/surface.hpp"

static BootInfo g_bootInfo;
static MemoryMap g_memoryMap;
static Surface g_frameBuffer;
static EfiConsole g_efiConsole;
static GraphicsConsole g_graphicsConsole;
Console* g_console;

static EFI_GUID g_efiFileInfoGuid = EFI_FILE_INFO_ID;
static EFI_GUID g_efiLoadedImageProtocolGuid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
static EFI_GUID g_efiSimpleFileSystemProtocolGuid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
static EFI_GUID g_efiGraphicsOutputProtocolUUID = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;

EFI_HANDLE              g_efiImage;
EFI_SYSTEM_TABLE*       g_efiSystemTable;
EFI_BOOT_SERVICES*      g_efiBootServices;
EFI_RUNTIME_SERVICES*   g_efiRuntimeServices;


static void EnumerateModes(EFI_GRAPHICS_OUTPUT_PROTOCOL* gop)
{
    printf("Available graphics modes:\n");
    for (unsigned i = 0; i != gop->Mode->MaxMode; ++i)
    {
        EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* info;
        UINTN size = sizeof(info);
        gop->QueryMode(gop, i, &size, &info);
        printf("Mode %02d: %d x %d - %d\n", i, info->HorizontalResolution, info->VerticalResolution, info->PixelFormat);
        if (info->PixelFormat == PixelBitMask)
        {
            printf("    R: %08x\n", info->PixelInformation.RedMask);
            printf("    G: %08x\n", info->PixelInformation.GreenMask);
            printf("    B: %08x\n", info->PixelInformation.BlueMask);
            printf("    X: %08x\n", info->PixelInformation.ReservedMask);
        }
    }
    printf("\nCurrent mode: %d\n", gop->Mode->Mode);
    printf("    Framebuffer: 0x%016llx\n", gop->Mode->FrameBufferBase);
    printf("    Size       : 0x%016lx\n", gop->Mode->FrameBufferSize);
}



// Look at this code and tell me EFI isn't insane
static EFI_STATUS LoadInitrd(const wchar_t* path)
{
    EFI_LOADED_IMAGE_PROTOCOL* image = NULL;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs = NULL;
    EFI_FILE_PROTOCOL* fileSystemRoot = NULL;
    EFI_FILE_PROTOCOL* file = NULL;
    EFI_FILE_INFO* info = NULL;
    void* initrd = NULL;
    UINTN size;
    EFI_STATUS status;

    // Get access to the boot file system
    status = g_efiBootServices->HandleProtocol(g_efiImage, &g_efiLoadedImageProtocolGuid, (void**)&image);
    if (EFI_ERROR(status) || !image)
        goto error;

    status = g_efiBootServices->HandleProtocol(image->DeviceHandle, &g_efiSimpleFileSystemProtocolGuid, (void**)&fs);
    if (EFI_ERROR(status) || !fs)
        goto error;

    // Open the file system
    status = fs->OpenVolume(fs, &fileSystemRoot);
    if (EFI_ERROR(status))
        goto error;

    // Open the initrd file
    status = fileSystemRoot->Open(fileSystemRoot, &file, (CHAR16*)path, EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(status))
        goto error;

    // Retrieve the initrd's size
    size = 0;
    status = file->GetInfo(file, &g_efiFileInfoGuid, &size, NULL);
    if (EFI_ERROR(status) && status != EFI_BUFFER_TOO_SMALL)
        goto error;

    status = g_efiBootServices->AllocatePool(EfiLoaderData, size, (void**)&info);
    if (EFI_ERROR(status))
        goto error;

    status = file->GetInfo(file, &g_efiFileInfoGuid, &size, info);
    if (EFI_ERROR(status))
        goto error;

    // Allocate memory to hold the initrd
    status = g_efiBootServices->AllocatePool(EfiLoaderData, info->FileSize, &initrd);
    if (EFI_ERROR(status))
        goto error;

    // Read the initrd into memory
    size = info->FileSize;
    status = file->Read(file, &size, initrd);
    if (EFI_ERROR(status) || size != info->FileSize)
        goto error;

    g_bootInfo.initrdAddress = (uintptr_t)initrd;
    g_bootInfo.initrdSize = size;

    goto exit;

error:
    if (initrd) g_efiBootServices->FreePool(initrd);

exit:
    if (info) g_efiBootServices->FreePool(info);
    if (file) file->Close(file);
    if (fileSystemRoot) fileSystemRoot->Close(fileSystemRoot);

    return status;
}



static EFI_STATUS BuildMemoryMap(MemoryMap* memoryMap, UINTN* mapKey)
{
    // Retrieve the memory map from EFI
    UINTN descriptorCount = 0;
    UINTN descriptorSize = 0;
    UINT32 descriptorVersion = 0;
    *mapKey = 0;

    EFI_MEMORY_DESCRIPTOR* descriptors = NULL;
    UINTN size = 0;

    EFI_STATUS status = EFI_BUFFER_TOO_SMALL;
    while (status == EFI_BUFFER_TOO_SMALL)
    {
        g_efiBootServices->FreePool(descriptors);
        descriptors = NULL;

        status = g_efiBootServices->AllocatePool(EfiLoaderData, size, (void**)&descriptors);
        if (EFI_ERROR(status))
            return status;

        status = g_efiBootServices->GetMemoryMap(&size, descriptors, mapKey, &descriptorSize, &descriptorVersion);
    }

    if (EFI_ERROR(status))
    {
        g_efiBootServices->FreePool(descriptors);
        return status;
    }

    descriptorCount = size / descriptorSize;


    // Convert EFI memory map to our own format
    EFI_MEMORY_DESCRIPTOR* descriptor = descriptors;
    for (UINTN i = 0; i != descriptorCount; ++i, descriptor = (EFI_MEMORY_DESCRIPTOR*)((uintptr_t)descriptor + descriptorSize))
    {
        MemoryType type;
        uint32_t flags;

        switch (descriptor->Type)
        {

        case EfiLoaderCode:
        case EfiBootServicesCode:
            type = MemoryType_Bootloader;
            flags = MemoryFlag_Code;
            break;

        case EfiLoaderData:
        case EfiBootServicesData:
            type = MemoryType_Bootloader;
            flags = 0;
            break;

        case EfiRuntimeServicesCode:
            type = MemoryType_Firmware;
            flags = MemoryFlag_Code;
            break;

        case EfiRuntimeServicesData:
            type = MemoryType_Firmware;
            flags = 0;
            break;

        case EfiConventionalMemory:
            type = MemoryType_Available;
            flags = 0;
            break;

        case EfiUnusableMemory:
            type = MemoryType_Unusable;
            flags = 0;
            break;

        case EfiACPIReclaimMemory:
            type = MemoryType_AcpiReclaimable;
            flags = 0;
            break;

        case EfiACPIMemoryNVS:
            type = MemoryType_AcpiNvs;
            flags = 0;
            break;

        case EfiPersistentMemory:
            type = MemoryType_Persistent;
            flags = 0;
            break;

        case EfiReservedMemoryType:
        case EfiMemoryMappedIO:
        case EfiMemoryMappedIOPortSpace:
        case EfiPalCode:
        default:
            type = MemoryType_Reserved;
            flags = 0;
            break;
        }

        memoryMap->AddBytes(type, flags, descriptor->PhysicalStart, descriptor->NumberOfPages * EFI_PAGE_SIZE);
    }

    return EFI_SUCCESS;
}



static EFI_STATUS ExitBootServices(MemoryMap* memoryMap)
{
    UINTN key;
    EFI_STATUS status = BuildMemoryMap(memoryMap, &key);
    if (EFI_ERROR(status))
    {
        printf("Failed to build memory map: %p\n", (void*)status);
        return status;
    }

    // memoryMap->Sanitize();
    // memoryMap->Print();

    status = g_efiBootServices->ExitBootServices(g_efiImage, key);
    if (EFI_ERROR(status))
    {
        printf("Failed to exit boot services: %p\n", (void*)status);
        return status;
    }

    // Clear out fields we can't use anymore
    g_efiSystemTable->ConsoleInHandle = 0;
    g_efiSystemTable->ConIn = NULL;
    g_efiSystemTable->ConsoleOutHandle = 0;
    g_efiSystemTable->ConOut = NULL;
    g_efiSystemTable->StandardErrorHandle = 0;
    g_efiSystemTable->StdErr = NULL;
    g_efiSystemTable->BootServices = NULL;

    g_efiBootServices = NULL;

    return EFI_SUCCESS;
}



static void CallGlobalConstructors()
{
    extern void (*__CTOR_LIST__[])();

    uintptr_t count = (uintptr_t) __CTOR_LIST__[0];

    if (count == (uintptr_t)-1)
    {
        count = 0;
        while (__CTOR_LIST__[count + 1])
            ++count;
    }

    for (uintptr_t i = count; i >= 1; --i)
    {
        __CTOR_LIST__[i]();
    }
}



static void CallGlobalDestructors()
{
    extern void (*__DTOR_LIST__[])();

    for (void (**p)() = __DTOR_LIST__ + 1; *p; ++p)
    {
        (*p)();
    }
}



extern "C" EFI_STATUS EFIAPI efi_main(EFI_HANDLE hImage, EFI_SYSTEM_TABLE* systemTable)
{
    if (!hImage || !systemTable)
        return EFI_INVALID_PARAMETER;

    // Keep these around, they are useful
    g_efiImage = hImage;
    g_efiSystemTable = systemTable;
    g_efiBootServices = systemTable->BootServices;
    g_efiRuntimeServices = systemTable->RuntimeServices;

    CallGlobalConstructors();

    // Welcome message
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* console = systemTable->ConOut;

    if (console)
    {
        g_efiConsole.Initialize(console);
        g_console = &g_efiConsole;
    }


    EFI_STATUS status;


    EFI_GRAPHICS_OUTPUT_PROTOCOL* gop = nullptr;
    status = g_efiBootServices->LocateProtocol(&g_efiGraphicsOutputProtocolUUID, nullptr, (void **)&gop);
    if (EFI_ERROR(status))
    {
        printf("*** Error retrieving EFI_GRAPHICS_OUTPUT_PROTCOL\n");
    }
    else
    {
        const int mode = gop->Mode->Mode;

        EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* info;
        UINTN size = sizeof(info);
        gop->QueryMode(gop, mode, &size, &info);

        g_frameBuffer.width = info->HorizontalResolution;
        g_frameBuffer.height = info->VerticalResolution;
        g_frameBuffer.pitch = info->PixelsPerScanLine * 4;  // TODO: here I assume 32 bpp
        g_frameBuffer.pixels = (void*)(uintptr_t)gop->Mode->FrameBufferBase;
        g_frameBuffer.format = PIXFMT_A8R8G8B8; // TODO: assumption

        g_graphicsConsole.Initialize(&g_frameBuffer);
        g_console = &g_graphicsConsole;
    }


    g_console->Rainbow();
    printf(" EFI Bootloader (" STRINGIZE(EFI_ARCH) ")\n\n");

    if (gop)
    {
        EnumerateModes(gop);
    }


    status = LoadInitrd(L"\\EFI\\rainbow\\initrd.img");
    if (EFI_ERROR(status))
    {
        printf("Failed to load initrd: %p\n", (void*)status);
        goto error;
    }

    ExitBootServices(&g_memoryMap);

    Boot(&g_bootInfo, &g_memoryMap);

error:
    printf("\nPress any key to exit");
    getchar();
    printf("\nExiting...");

    CallGlobalDestructors();

    return status;
}
