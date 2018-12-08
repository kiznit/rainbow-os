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
#include "eficonsole.hpp"
#include "log.hpp"
#include "x86.hpp"

// Intel's UEFI header do not properly define EFI_MEMORY_DESCRIPTOR for GCC.
// This check ensures that it is.
static_assert(offsetof(EFI_MEMORY_DESCRIPTOR, PhysicalStart) == 8);

EFI_HANDLE g_efiImage;
EFI_SYSTEM_TABLE* ST;
EFI_BOOT_SERVICES* BS;


EFI_STATUS InitDisplays();
EFI_STATUS LoadFile(const wchar_t* path, void*& fileData, size_t& fileSize);



static void* AllocatePages(size_t pageCount, physaddr_t maxAddress)
{
    EFI_PHYSICAL_ADDRESS memory = maxAddress - 1;
    EFI_STATUS status = BS->AllocatePages(AllocateMaxAddress, EfiLoaderData, pageCount, &memory);
    if (EFI_ERROR(status))
    {
        return nullptr;
    }

    return (void*)memory;
}



/*
    NOTE: Do not print anything in this function. If the current console is 'EfiConsole'
          and we print something, ExitBootServices() will fail with EFI_INVALID_PARAMETER.
          We will then enter infinite recursion. I suspect that calling ST->ConOut->OutputString
          allocates memory and that changes the memory map (key).
*/
static EFI_STATUS BuildMemoryMap(UINTN* mapKey)
{
    UINTN descriptorCount = 0;
    UINTN descriptorSize = 0;
    UINT32 descriptorVersion = 0;
    *mapKey = 0;

    UINTN size = 0;
    EFI_MEMORY_DESCRIPTOR* descriptors = nullptr;
    EFI_STATUS status;

    while ((status = BS->GetMemoryMap(&size, descriptors, mapKey, &descriptorSize, &descriptorVersion)) == EFI_BUFFER_TOO_SMALL)
    {
        BS->FreePool(descriptors);
        descriptors = nullptr;
        status = BS->AllocatePool(EfiLoaderData, size, (void**)&descriptors);
        if (EFI_ERROR(status)) break;
    }

    if (EFI_ERROR(status))
    {
        BS->FreePool(descriptors);
        return status;
    }

    descriptorCount = size / descriptorSize;

    //Log("\nMemoryMap:\n");

    const EFI_MEMORY_DESCRIPTOR* descriptor = descriptors;
    for (UINTN i = 0; i != descriptorCount; ++i, descriptor = (EFI_MEMORY_DESCRIPTOR*)((uintptr_t)descriptor + descriptorSize))
    {
        //Log("    %X - %X: %x %X\n", descriptor->PhysicalStart, descriptor->PhysicalStart + descriptor->NumberOfPages * EFI_PAGE_SIZE, descriptor->Type, descriptor->Attribute);
    }

    return EFI_SUCCESS;
}



static EFI_STATUS ExitBootServices()
{
    UINTN key;
    EFI_STATUS status = BuildMemoryMap(&key);
    if (EFI_ERROR(status))
    {
        return status;
    }

    status = BS->ExitBootServices(g_efiImage, key);
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

    Log("Rainbow-OS UEFI Bootloader (" STRINGIZE(BOOT_ARCH) ")\n\n");

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


    for(;;);

    return EFI_SUCCESS;
}
