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
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>

#include "boot.hpp"
#include "memory.hpp"

static BootInfo g_bootInfo;
static MemoryMap g_memoryMap;

static EFI_GUID g_efiFileInfoGuid = EFI_FILE_INFO_ID;
static EFI_GUID g_efiLoadedImageProtocolGuid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
static EFI_GUID g_efiSimpleFileSystemProtocolGuid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;

static EFI_HANDLE                       g_efiImage;
static EFI_SYSTEM_TABLE*                g_efiSystemTable;
static EFI_BOOT_SERVICES*               g_efiBootServices;
static EFI_RUNTIME_SERVICES*            g_efiRuntimeServices;



extern "C" int _libc_print(const char* string)
{
    if (!g_efiSystemTable)
        return EOF;

    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* output = g_efiSystemTable->ConOut;
    if (!output)
        return EOF;

    size_t length = 0;

    CHAR16 buffer[200];
    size_t count = 0;

    for (const char* p = string; *p; ++p, ++length)
    {
        const unsigned char c = *p;

        if (c == '\n')
            buffer[count++] = '\r';

        buffer[count++] = c;

        if (count >= ARRAY_LENGTH(buffer) - 3)
        {
            buffer[count] = '\0';
            output->OutputString(output, buffer);
            count = 0;
        }
    }

    if (count > 0)
    {
        buffer[count] = '\0';
        output->OutputString(output, buffer);
    }

    return length;
}



extern "C" int getchar()
{
    if (!g_efiSystemTable || !g_efiBootServices)
        return EOF;

    EFI_SIMPLE_TEXT_INPUT_PROTOCOL* input = g_efiSystemTable->ConIn;
    if (!input)
        return EOF;

    for (;;)
    {
        EFI_STATUS status;

        size_t index;
        status = g_efiBootServices->WaitForEvent(1, &input->WaitForKey, &index);
        if (EFI_ERROR(status))
        {
            return EOF;
        }

        EFI_INPUT_KEY key;
        status = input->ReadKeyStroke(input, &key);
        if (EFI_ERROR(status))
        {
            if (status == EFI_NOT_READY)
                continue;

            return EOF;
        }

        return key.UnicodeChar;
    }
}



static void InitConsole(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* console)
{
    // Mode 0 is always 80x25 text mode and is always supported
    // Mode 1 is always 80x50 text mode and isn't always supported
    // Modes 2+ are differents on every device
    size_t mode = 0;
    size_t width = 80;
    size_t height = 25;

    for (size_t m = 0; ; ++m)
    {
        size_t  w, h;
        EFI_STATUS status = console->QueryMode(console, m, &w, &h);
        if (EFI_ERROR(status))
        {
            // Mode 1 might return EFI_UNSUPPORTED, we still want to check modes 2+
            if (m > 1)
                break;
        }
        else
        {
            if (w * h > width * height)
            {
                mode = m;
                width = w;
                height = h;
            }
        }
    }

    console->SetMode(console, mode);

    // Some firmware won't clear the screen and/or reset the text colors on SetMode().
    // This is presumably more likely to happen when the selected mode is the current one.
    console->SetAttribute(console, EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLACK));
    console->ClearScreen(console);
    console->EnableCursor(console, FALSE);
    console->SetCursorPosition(console, 0, 0);
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



extern "C" EFI_STATUS EFIAPI efi_main(EFI_HANDLE hImage, EFI_SYSTEM_TABLE* systemTable)
{
    if (!hImage || !systemTable)
        return EFI_INVALID_PARAMETER;

    // Keep these around, they are useful
    g_efiImage = hImage;
    g_efiSystemTable = systemTable;
    g_efiBootServices = systemTable->BootServices;
    g_efiRuntimeServices = systemTable->RuntimeServices;

    // Welcome message
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* console = systemTable->ConOut;

    if (console)
    {
        InitConsole(console);

        console->SetAttribute(console, EFI_TEXT_ATTR(EFI_RED,         EFI_BLACK)); console->OutputString(console, (CHAR16*)L"R");
        console->SetAttribute(console, EFI_TEXT_ATTR(EFI_LIGHTRED,    EFI_BLACK)); console->OutputString(console, (CHAR16*)L"a");
        console->SetAttribute(console, EFI_TEXT_ATTR(EFI_YELLOW,      EFI_BLACK)); console->OutputString(console, (CHAR16*)L"i");
        console->SetAttribute(console, EFI_TEXT_ATTR(EFI_LIGHTGREEN,  EFI_BLACK)); console->OutputString(console, (CHAR16*)L"n");
        console->SetAttribute(console, EFI_TEXT_ATTR(EFI_LIGHTCYAN,   EFI_BLACK)); console->OutputString(console, (CHAR16*)L"b");
        console->SetAttribute(console, EFI_TEXT_ATTR(EFI_LIGHTBLUE,   EFI_BLACK)); console->OutputString(console, (CHAR16*)L"o");
        console->SetAttribute(console, EFI_TEXT_ATTR(EFI_LIGHTMAGENTA,EFI_BLACK)); console->OutputString(console, (CHAR16*)L"w");
        console->SetAttribute(console, EFI_TEXT_ATTR(EFI_LIGHTGRAY,   EFI_BLACK));

        printf(" EFI Bootloader (" STRINGIZE(EFI_ARCH) ")\n\n");
    }


    EFI_STATUS status;

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

    return status;
}
