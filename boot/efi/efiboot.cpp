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
#include <string.h>

#include "boot.hpp"
#include "efi.hpp"
#include "elf.hpp"
#include "memory.hpp"

#include <rainbow/boot.h>

// Globals
efi::handle_t           g_efiImage;
efi::SystemTable*       g_efiSystemTable;
efi::BootServices*      g_efiBootServices;
efi::RuntimeServices*   g_efiRuntimeServices;

static MemoryMap g_memoryMap;
static BootInfo g_bootInfo;


// Smart file pointer
class scoped_file_ptr
{
public:
    scoped_file_ptr(efi::FileProtocol* file = NULL) : m_file(file) {}
    ~scoped_file_ptr()                              { close(); }

    void close()                                    { if (m_file) { m_file->Close(); m_file = NULL; } }
    void reset(efi::FileProtocol* file = NULL)      { m_file = file; }
    efi::FileProtocol*& raw()                       { return m_file; }
    efi::FileProtocol* operator->() const           { return m_file; }
    efi::FileProtocol* get() const                  { return m_file; }

private:
    efi::FileProtocol* m_file;
};



class MemoryBuffer
{
public:

    MemoryBuffer(size_t size)   { m_buffer = (char*)malloc(size); m_size = size; }
    ~MemoryBuffer()             { free(m_buffer); }

    bool Valid() const          { return m_buffer != NULL; }

    char* begin()               { return m_buffer; }
    char* end()                 { return m_buffer + m_size; }
    size_t size()               { return m_size; }


private:

    char* m_buffer;
    size_t m_size;
};



static efi::status_t BuildMemoryMap(size_t* mapKey)
{
    size_t descriptorCount;
    size_t descriptorSize;
    uint32_t descriptorVersion;

    efi::MemoryDescriptor* memoryMap = g_efiBootServices->GetMemoryMap(&descriptorCount, &descriptorSize, &descriptorVersion, mapKey);
    if (!memoryMap)
    {
        printf("Failed to retrieve memory map!\n");
        return EFI_OUT_OF_RESOURCES;
    }

    efi::MemoryDescriptor* descriptor = memoryMap;
    for (size_t i = 0; i != descriptorCount; ++i, descriptor = (efi::MemoryDescriptor*)((uintptr_t)descriptor + descriptorSize))
    {
        MemoryType type = MemoryType_Reserved;

        switch (descriptor->type)
        {
        case efi::EfiUnusableMemory:
            type = MemoryType_Unusable;
            break;

        case efi::EfiLoaderCode:
        case efi::EfiLoaderData:
        case efi::EfiConventionalMemory:
            if (descriptor->attribute & efi::EFI_MEMORY_WB)
                type = MemoryType_Available;
            else
                type = MemoryType_Reserved;
            break;

        case efi::EfiBootServicesCode:
        case efi::EfiBootServicesData:
            // Work around buggy firmware that call boot services after we exited them.
            if (descriptor->attribute & efi::EFI_MEMORY_WB)
                type = MemoryType_Bootloader;
            else
                type = MemoryType_Reserved;
            break;

        case efi::EfiRuntimeServicesCode:
        case efi::EfiRuntimeServicesData:
            type = MemoryType_FirmwareRuntime;
            break;

        case efi::EfiACPIReclaimMemory:
            type = MemoryType_AcpiReclaimable;
            break;

        case efi::EfiACPIMemoryNVS:
            type = MemoryType_AcpiNvs;
            break;

        case efi::EfiReservedMemoryType:
        case efi::EfiMemoryMappedIO:
        case efi::EfiMemoryMappedIOPortSpace:
        case efi::EfiPalCode:
            type = MemoryType_Reserved;
            break;
        }

        g_memoryMap.AddPages(type, descriptor->physicalStart, descriptor->numberOfPages);
    }

    return EFI_SUCCESS;
}



static efi::status_t ExitBootServices()
{
    efi::status_t status;

    size_t key;
    status = BuildMemoryMap(&key);
    if (EFI_ERROR(status))
    {
        printf("Failed to build memory map: %p\n", (void*)status);
        return status;
    }

    g_memoryMap.Print();

    // status = g_efiSystemTable->ExitBootServices(g_efiImage, key);
    // if (EFI_ERROR(status))
    // {
    //     printf("Failed to exit boot services: %p\n", (void*)status);
    //     return status;
    // }

    //g_efiBootServices = NULL;

    return EFI_SUCCESS;
}



static efi::status_t LoadAndExecuteKernel(efi::FileProtocol* fileSystemRoot, const wchar_t* path)
{
    efi::status_t status;

    scoped_file_ptr file;
    status = fileSystemRoot->Open(&file.raw(), path);
    if (EFI_ERROR(status))
        return status;

    efi::FileInfo info;
    status = file->GetInfo(&info);
    if (EFI_ERROR(status))
        return status;

    MemoryBuffer buffer(info.fileSize);
    if (!buffer.Valid())
        return EFI_OUT_OF_RESOURCES;

    size_t readSize = info.fileSize;

    status = file->Read(buffer.begin(), &readSize);
    if (EFI_ERROR(status) || readSize != info.fileSize)
        return status;

    ElfLoader elf(buffer.begin(), readSize);
    if (!elf.Valid())
    {
        printf("Unsupported: \"%w\" is not a valid elf file\n", path);
        return EFI_UNSUPPORTED;
    }

    if (elf.GetType() != ET_EXEC)
    {
        printf("Unsupported: \"%w\" is not an executable\n", path);
        return EFI_UNSUPPORTED;
    }

    // unsigned int size = elf.GetMemorySize();
    // unsigned int alignment = elf.GetMemoryAlignment();

    // void* memory = NULL;

    // if (alignment <= efi::PAGE_SIZE)
    // {
    //     const int pageCount = (size + efi::PAGE_SIZE - 1) >> efi::PAGE_SHIFT;

    //     physaddr_t address = g_efiBootServices->AllocatePages(pageCount, RAINBOW_KERNEL_BASE_ADDRESS);
    //     if (address != (physaddr_t)-1)
    //     {
    //         memory = (void*)address;
    //     }
    // }

    // if (!memory)
    // {
    //     printf("Could not allocate memory to load launcher (size: %u, alignment: %u)\n", size, alignment);
    //     return EFI_OUT_OF_RESOURCES;
    // }

    // printf("Launcher memory allocated at %p\n", memory);

    // void* entry = elf.Load(memory);
    // if (entry == NULL)
    // {
    //     printf("Error loading launcher\n");
    //     return EFI_UNSUPPORTED;
    // }

    // printf("launcher_main() at %p\n", entry);

    status = ExitBootServices();
    if (EFI_ERROR(status))
    {
        return status;
    }

    // g_memoryMap.Sanitize();

    // // putchar('\n');
    // // g_memoryMap.Print();
    // // putchar('\n');
    // // g_modules.Print();
    // // putchar('\n');

    // // TEMP: execute Launcher to see that it works properly
    // typedef const char* (*launcher_entry_t)(char**);
    // launcher_entry_t launcher_main = (launcher_entry_t)entry;
    // char* out;
    // const char* result = launcher_main(&out);
    // printf("RESULT: %p, out: %p\n", result, out);
    // printf("Which is: '%s', [%d, %d, %d, ..., %d]\n", result, out[0], out[1], out[2], out[99]);

    // TODO: if we are in 64 bits, we need to switch to 32 bits here

    // // Jump to launcher
    // typedef void (*launcher_entry_t)(BootInfo*);
    // launcher_entry_t launcher_main = (launcher_entry_t)entry;
    // launcher_main(&bootInfo);

    // We don't expect launcher to return... If it does, return an error.
    return EFI_UNSUPPORTED;
}



static efi::status_t Boot()
{
    efi::status_t status;
    efi::LoadedImageProtocol* image = NULL;

    status = g_efiBootServices->HandleProtocol(g_efiImage, &image);
    if (EFI_ERROR(status))
    {
        printf("Could not open EfiLoadedImageProtocol\n");
        return status;
    }

    efi::SimpleFileSystemProtocol* fs;
    status = g_efiBootServices->HandleProtocol(image->deviceHandle, &fs);
    if (EFI_ERROR(status))
        return status;

    scoped_file_ptr fileSystemRoot;
    status = fs->OpenVolume(&fileSystemRoot.raw());
    if (EFI_ERROR(status))
        return status;

    // NOTE: if LoadAndExecuteKernel() returns, it means it failed to start the kernel!
    LoadAndExecuteKernel(fileSystemRoot.get(), L"\\rainbow\\kernel");

#if defined(__i386__) || defined(__x86_64__)
    if (VerifyCPU_x86_64())
    {
        LoadAndExecuteKernel(fileSystemRoot.get(), L"\\rainbow\\kernel_x86_64");
    }

    if (VerifyCPU_ia32())
    {
        LoadAndExecuteKernel(fileSystemRoot.get(), L"\\rainbow\\kernel_ia32");
    }
#endif

    return EFI_UNSUPPORTED;
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



static void InitConsole()
{
    efi::SimpleTextOutputProtocol* output = g_efiSystemTable->conOut;

    if (!output)
        return;

    // Mode 0 is always 80x25 text mode and is always supported
    // Mode 1 is always 80x50 text mode and isn't always supported
    // Modes 2+ are differents on every device
    size_t mode = 0;
    size_t width = 80;
    size_t height = 25;

    for (size_t m = 0; ; ++m)
    {
        size_t  w, h;
        efi::status_t status = output->QueryMode(output, m, &w, &h);
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

    output->SetMode(output, mode);

    // Some firmware won't clear the screen and/or reset the text colors on SetMode().
    // This is presumably more likely to happen when the selected mode is the existing one.
    output->SetAttribute(output, EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLACK));
    output->ClearScreen(output);
    output->EnableCursor(output, efi::FALSE);
    output->SetCursorPosition(output, 0, 0);
}



static void Initialize(efi::handle_t hImage, efi::SystemTable* systemTable)
{
    g_efiImage = hImage;
    g_efiSystemTable = systemTable;
    g_efiBootServices = systemTable->bootServices;
    g_efiRuntimeServices = systemTable->runtimeServices;

    CallGlobalConstructors();

    InitConsole();
}



static void Shutdown()
{
    printf("\nPress any key to exit");
    getchar();
    printf("\nExiting...");

    CallGlobalDestructors();
}



extern "C" efi::status_t EFIAPI efi_main(efi::handle_t hImage, efi::SystemTable* systemTable)
{
    if (!hImage || !systemTable)
        return EFI_INVALID_PARAMETER;

    Initialize(hImage, systemTable);

    g_bootInfo.version = RAINBOW_BOOT_VERSION;
    g_bootInfo.firmware = Firmware_EFI;

    // Welcome message
    efi::SimpleTextOutputProtocol* output = g_efiSystemTable->conOut;
    if (output)
    {
        const int32_t backupAttributes = output->mode->attribute;

        output->SetAttribute(output, EFI_TEXT_ATTR(EFI_RED,         EFI_BLACK)); putchar('R');
        output->SetAttribute(output, EFI_TEXT_ATTR(EFI_LIGHTRED,    EFI_BLACK)); putchar('a');
        output->SetAttribute(output, EFI_TEXT_ATTR(EFI_YELLOW,      EFI_BLACK)); putchar('i');
        output->SetAttribute(output, EFI_TEXT_ATTR(EFI_LIGHTGREEN,  EFI_BLACK)); putchar('n');
        output->SetAttribute(output, EFI_TEXT_ATTR(EFI_LIGHTCYAN,   EFI_BLACK)); putchar('b');
        output->SetAttribute(output, EFI_TEXT_ATTR(EFI_LIGHTBLUE,   EFI_BLACK)); putchar('o');
        output->SetAttribute(output, EFI_TEXT_ATTR(EFI_LIGHTMAGENTA,EFI_BLACK)); putchar('w');

        output->SetAttribute(output, backupAttributes);

        printf(" EFI Bootloader (" STRINGIZE(EFI_ARCH) ")\n\n", (int)sizeof(void*)*8);
    }


    efi::status_t status = Boot();

    printf("Boot() returned with status %p\n", (void*)status);

    Shutdown();

    return status;
}
