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

EFI_HANDLE g_efiImage;
EFI_SYSTEM_TABLE* ST;
EFI_BOOT_SERVICES* BS;


void InitDisplays();
EFI_STATUS LoadFile(const wchar_t* path, void*& fileData, size_t& fileSize);



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

    void* kernelData;
    size_t kernelSize;

    auto status = LoadFile(L"\\EFI\\rainbow\\kernel", kernelData, kernelSize);
    if (EFI_ERROR(status))
    {
        Log("Failed to load kernel: %p\n", status);
        //todo: return status;
    }

    Log("Kernel loaded at: %p, size: %x\n", kernelData, kernelSize);

    for(;;);

    return EFI_SUCCESS;
}
