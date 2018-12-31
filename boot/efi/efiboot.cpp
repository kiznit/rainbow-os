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
#include <rainbow/uefi.h>
#include "eficonsole.hpp"
#include "log.hpp"
#include "memory.hpp"


// Intel's UEFI header do not properly define EFI_MEMORY_DESCRIPTOR for GCC.
// This check ensures that it is.
static_assert(offsetof(EFI_MEMORY_DESCRIPTOR, PhysicalStart) == 8);

EFI_HANDLE g_efiImage;
EFI_SYSTEM_TABLE* ST;
EFI_BOOT_SERVICES* BS;
MemoryMap g_memoryMap;


EFI_STATUS InitDisplays();
EFI_STATUS LoadFile(const wchar_t* path, void*& fileData, size_t& fileSize);



void* AllocatePages(size_t pageCount, physaddr_t maxAddress)
{
    EFI_PHYSICAL_ADDRESS memory = maxAddress - 1;
    EFI_STATUS status = BS->AllocatePages(AllocateMaxAddress, EfiLoaderData, pageCount, &memory);
    if (EFI_ERROR(status))
    {
        return nullptr;
    }

    return (void*)memory;
}



// Convert EFI memory map to our own format
static void BuildMemoryMap(const EFI_MEMORY_DESCRIPTOR* descriptors, size_t descriptorCount, size_t descriptorSize)
{
    const EFI_MEMORY_DESCRIPTOR* descriptor = descriptors;
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

        g_memoryMap.AddBytes(type, flags, descriptor->PhysicalStart, descriptor->NumberOfPages * EFI_PAGE_SIZE);
    }
}



static EFI_STATUS ExitBootServices()
{
    UINTN size = 0;
    UINTN allocatedSize = 0;
    EFI_MEMORY_DESCRIPTOR* descriptors = nullptr;
    UINTN memoryMapKey = 0;
    UINTN descriptorSize = 0;
    UINT32 descriptorVersion = 0;

    // 1) Retrieve the memory map from the firmware
    EFI_STATUS status;
    while ((status = BS->GetMemoryMap(&size, descriptors, &memoryMapKey, &descriptorSize, &descriptorVersion)) == EFI_BUFFER_TOO_SMALL)
    {
        // Extra space to play safe with "partial shutdown" when calling ExitBootServices().
        size += descriptorSize * 10;

        descriptors = (EFI_MEMORY_DESCRIPTOR*)realloc(descriptors, size);
        if (!descriptors)
        {
            return EFI_OUT_OF_RESOURCES;
        }

        allocatedSize = size;
    }

    // 2) Exit boot services - it is possible for the firmware to modify the memory map
    // during a call to ExitBootServices(). A so-called "partial shutdown".
    // When that happens, ExitBootServices() will return EFI_INVALID_PARAMETER.
    while ((status = BS->ExitBootServices(g_efiImage, memoryMapKey)) == EFI_INVALID_PARAMETER)
    {
        // Memory map changed during ExitBootServices(), the only APIs we are allowed to
        // call at this point are GetMemoryMap() and ExitBootServices().
        size = allocatedSize;
        status = BS->GetMemoryMap(&size, descriptors, &memoryMapKey, &descriptorSize, &descriptorVersion);
        if (EFI_ERROR(status))
        {
            return status;
        }
    }

    if (EFI_ERROR(status))
    {
        return status;
    }

    // Clear out fields we can't use anymore
    ST->ConsoleInHandle = 0;
    ST->ConIn = NULL;
    ST->ConsoleOutHandle = 0;
    ST->ConOut = NULL;
    ST->StandardErrorHandle = 0;
    ST->StdErr = NULL;
    ST->BootServices = NULL;

    BS = NULL;


    BuildMemoryMap(descriptors, size / descriptorSize, descriptorSize);

    return EFI_SUCCESS;
}



extern "C" EFI_STATUS efi_main(EFI_HANDLE hImage, EFI_SYSTEM_TABLE* systemTable)
{
    g_efiImage = hImage;
    ST = systemTable;
    BS = systemTable->BootServices;

    EfiConsole efiConsole;
    g_console = &efiConsole;

    // It is in theory possible for EFI_BOOT_SERVICES::AllocatePages() to return
    // an allocation at address 0. We do not want this to happen as we use NULL
    // to indicate errors / out-of-memory condition. To ensure it doesn't happen,
    // we attempt to allocate the first memory page. We do not care whether or
    // not it succeeds.
    AllocatePages(1, MEMORY_PAGE_SIZE);

    // Initialize displays with usable graphics modes
    EFI_STATUS status = InitDisplays();

    g_console->Rainbow();
    Log(" UEFI Bootloader (" STRINGIZE(BOOT_ARCH) ")\n\n");

    if (EFI_ERROR(status))
    {
        Fatal("Failed to initialize graphics\n");
    }


    void* kernelData;
    size_t kernelSize;

    status = LoadFile(L"\\EFI\\rainbow\\kernel", kernelData, kernelSize);
    if (EFI_ERROR(status))
    {
        Fatal("Failed to load kernel: %p\n", status);
    }

    Log("Kernel loaded at: %p, size: %x\n", kernelData, kernelSize);

    status = ExitBootServices();
    if (EFI_ERROR(status))
    {
        Fatal("Failed to exit boot services: %p\n", status);
    }

    Boot(&g_memoryMap, kernelData, kernelSize);

    return EFI_SUCCESS;
}
