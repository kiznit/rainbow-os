/*
    Copyright (c) 2015, Thierry Tremblay
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

#include "efi.hpp"

#include <stdio.h>
#include <string.h>

#include "elf.hpp"
#include "memory.hpp"
#include "module.hpp"


static MemoryMap g_memoryMap;
static Modules g_modules;


#define STRINGIZE_DELAY(x) #x
#define STRINGIZE(x) STRINGIZE_DELAY(x)

#define ARRAY_LENGTH(array)     (sizeof(array) / sizeof((array)[0]))


// EFI Globals

static efi::handle_t            g_efiImage;
static efi::SystemTable*        g_efiSystemTable;
static efi::BootServices*       g_efiBootServices;
static efi::RuntimeServices*    g_efiRuntimeServices;



/*
    libc support
*/

extern "C" void __rainbow_putc(unsigned char c)
{
    efi::SimpleTextOutputProtocol* output = g_efiSystemTable->conOut;

    if (!output)
        return;

    if (c == '\n')
    {
        wchar_t string[3] = { '\r', '\n', '\0' };
        output->OutputString(output, string);
    }
    else
    {
        wchar_t string[2] = { c, '\0' };
        output->OutputString(output, string);
    }
}



static int getchar()
{
    efi::SimpleTextInputProtocol* input = g_efiSystemTable->conIn;

    if (!g_efiBootServices || !input)
        return EOF;

    for (;;)
    {
        efi::status_t status;

        size_t index;
        status = g_efiBootServices->WaitForEvent(1, &input->waitForKey, &index);
        if (EFI_ERROR(status))
        {
            return EOF;
        }

        efi::InputKey key;
        status = input->ReadKeyStroke(input, &key);
        if (EFI_ERROR(status))
        {
            if (status == EFI_NOT_READY)
                continue;

            return EOF;
        }

        return key.unicodeChar;
    }
}



extern "C" void* malloc(size_t size)
{
    if (g_efiBootServices)
        return g_efiBootServices->Allocate(size);
    else
        return NULL;
}


extern "C" void free(void* p)
{
    if (p && g_efiBootServices)
        g_efiBootServices->Free(p);
}



/*
    Screen initialization
*/

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
    output->SetAttribute(output, EFI_TEXT_ATTR(EFI_GREEN, EFI_BLACK));
    output->ClearScreen(output);
}



static efi::status_t LoadModule(efi::FileProtocol* root, const wchar_t* szPath, const char* name)
{
    efi::status_t status;

    efi::FileProtocol* file;
    status = root->Open(&file, szPath);
    if (EFI_ERROR(status))
        return status;

    efi::FileInfo info;
    status = file->GetInfo(&info);
    if (EFI_ERROR(status))
    {
        file->Close();
        return status;
    }

    const uint64_t fileSize = info.fileSize;

    // In theory I should be able to call AllocatePages with a custom memory type (0x80000000)
    // to track module data. In practice, doing so crashes my main development system.
    // Motherboard/firmware info: Hero Hero Maximus VI (build 1603 2014/09/19).
    int pageCount = (fileSize + efi::PAGE_SIZE - 1) & ~efi::PAGE_MASK;

    void* fileData = g_efiBootServices->AllocatePages(pageCount, 0xF0000000);
    if (!fileData)
    {
        file->Close();
        return EFI_OUT_OF_RESOURCES;
    }

    size_t readSize = fileSize;

    status = file->Read(fileData, &readSize);
    if (EFI_ERROR(status) || readSize != fileSize)
    {
        printf("Failed to load module \"%s\"\n", name);
        file->Close();
        return status;
    }

    const efi::PhysicalAddress start = (uintptr_t) fileData;
    const efi::PhysicalAddress end = start + readSize;

    g_modules.AddModule(name, start, end);

    return EFI_SUCCESS;
}


struct ModuleEntry
{
    const wchar_t* path;
    const char* name;
};

static const ModuleEntry s_modules[] =
{
    { L"\\rainbow\\launcher", "/rainbow/launcher" },
#if defined(__i386__) || defined(__x86_64__)
    { L"\\rainbow\\kernel_ia32", "/rainbow/kernel_ia32" },
    { L"\\rainbow\\kernel_x86_64", "/rainbow/kernel_x86_64" },
#endif
};


static efi::status_t LoadModules(efi::handle_t hDevice)
{
    efi::status_t status;

    efi::SimpleFileSystemProtocol* fs;
    status = g_efiBootServices->OpenProtocol(hDevice, &fs);
    if (EFI_ERROR(status))
        return status;

    efi::FileProtocol* root;
    status = fs->OpenVolume(&root);
    if (EFI_ERROR(status))
    {
        g_efiBootServices->CloseProtocol(hDevice, &fs);
        return status;
    }

    for (size_t i = 0; i != ARRAY_LENGTH(s_modules); ++i)
    {
        status = LoadModule(root, s_modules[i].path, s_modules[i].name);
        if (EFI_ERROR(status))
        {
            root->Close();
            g_efiBootServices->CloseProtocol(hDevice, &fs);
            return status;
        }
    }

    root->Close();
    g_efiBootServices->CloseProtocol(hDevice, &fs);

    return EFI_SUCCESS;
}



static efi::status_t BuildMemoryMap()
{
    size_t descriptorCount;
    size_t mapKey;
    size_t descriptorSize;
    uint32_t descriptorVersion;

    efi::MemoryDescriptor* memoryMap = g_efiBootServices->GetMemoryMap(&descriptorCount, &descriptorSize, &descriptorVersion, &mapKey);
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

        const physaddr_t start = descriptor->physicalStart;
        const physaddr_t end = start + descriptor->numberOfPages * efi::PAGE_SIZE;

        g_memoryMap.AddEntry(type, start, end);
    }

    free(memoryMap);

    // Now account for the bootloader modules
    for (Modules::const_iterator module = g_modules.begin(); module != g_modules.end(); ++module)
    {
        g_memoryMap.AddEntry(MemoryType_Bootloader, module->start, module->end);
    }

    g_memoryMap.Sanitize();

    return EFI_SUCCESS;
}





static int LoadElf32(const char* file, size_t size)
{
    Elf32Loader elf(file, size);

    if (!elf.Valid())
    {
        printf("Invalid ELF file\n");
        return -1;
    }

    if (elf.GetMemoryAlignment() > MEMORY_PAGE_SIZE)
    {
        printf("ELF aligment not supported\n");
        return -2;
    }

    // Allocate memory (we ignore alignment here and assume it is 4096 or less)
    char* memory = (char*) g_memoryMap.Alloc(MemoryZone_Normal, MemoryType_Unusable, elf.GetMemorySize());

    printf("Memory allocated at %p\n", memory);


    void* entry = elf.Load(memory);

    printf("ENTRY AT %p\n", entry);


    // TEMP: execute Launcher to see that it works properly
    typedef const char* (*launcher_entry_t)(char**) __attribute__((sysv_abi));

    launcher_entry_t launcher_main = (launcher_entry_t)entry;
    char* out;
    const char* result = launcher_main(&out);

    printf("RESULT: %p, out: %p\n", result, out);
    printf("Which is: '%s', [%d, %d, %d, ..., %d]\n", result, out[0], out[1], out[2], out[99]);

    return 0;
}



static int LoadLauncher()
{
    const ModuleInfo* launcher = NULL;

    for (Modules::const_iterator module = g_modules.begin(); module != g_modules.end(); ++module)
    {
        //todo: use case insensitive strcmp
        if (strcmp(module->name, "/rainbow/launcher") == 0)
        {
            launcher = module;
            break;
        }
    }

    if (!launcher)
    {
        printf("Module not found: launcher");
        return -1;
    }

    if (launcher->end > 0x100000000)
    {
        printf("Module launcher is in high memory (>4 GB) and can't be loaded");
        return -1;
    }

    int result = LoadElf32((char*)launcher->start, launcher->end - launcher->start);
    if (result < 0)
    {
        printf("Failed to load launcher\n");
        return result;
    }

    return 0;
}



// static EFI_STATUS ExitBootServices(EFI_HANDLE hImage)
// {
//     UINTN descriptorCount;
//     UINTN mapKey;
//     UINTN descriptorSize;
//     UINT32 descriptorVersion;

//     EFI_MEMORY_DESCRIPTOR* memoryMap = LibMemoryMap(&descriptorCount, &mapKey, &descriptorSize, &descriptorVersion);

//     if (!memoryMap)
//     {
//         printf("Failed to retrieve memory map!\n");
//         return EFI_OUT_OF_RESOURCES;
//     }

//     // Map runtime services to this Virtual Memory Address (vma)
//     bool mappedAnything = false;
//     physaddr_t vma = 0x80000000;

//     EFI_MEMORY_DESCRIPTOR* descriptor = memoryMap;
//     for (UINTN i = 0; i != descriptorCount; ++i, descriptor = NextMemoryDescriptor(descriptor, descriptorSize))
//     {
//         if (descriptor->Attribute & EFI_MEMORY_RUNTIME)
//         {
//             //const physaddr_t start = descriptor->PhysicalStart;
//             const physaddr_t size = descriptor->NumberOfPages * EFI_PAGE_SIZE;
//             //const physaddr_t end = start + size;

//             descriptor->VirtualStart = vma;
//             mappedAnything = true;

//             vma = vma + size;
//         }
//     }

// //TODO
//     (void)hImage;
//     // EFI_STATUS status = BS->ExitBootServices(hImage, mapKey);
//     // if (EFI_ERROR(status))
//     // {
//     //     printf("Failed to exit boot services: %p\n", (void*)status);
//     //     return status;
//     // }

//     if (mappedAnything)
//     {
//         EFI_STATUS status = RT->SetVirtualAddressMap(descriptorCount * descriptorSize, descriptorSize, descriptorVersion, memoryMap);
//         if (EFI_ERROR(status))
//         {
//             printf("Failed to set virtual address map: %p\n", (void*)status);
//             return status;
//         }
//     }

//     FreePool(memoryMap);

//     return EFI_SUCCESS;
// }




static efi::status_t Boot()
{
    efi::status_t status;
    efi::LoadedImageProtocol* image = NULL;

    status = g_efiBootServices->OpenProtocol(g_efiImage, &image);
    if (EFI_ERROR(status))
    {
        printf("Could not open EfiLoadedImageProtocol\n");
        return status;
    }

    //printf("Boot device     : %w\n", DevicePathToStr(DevicePathFromHandle(image->hDevice)));
    //printf("Bootloader      : %w\n", image->filePath->ToString());

    status = LoadModules(image->deviceHandle);
    if (EFI_ERROR(status))
    {
        printf("Could not load modules\n");
        return status;
    }

    g_efiBootServices->CloseProtocol(g_efiImage, &image);


    status = BuildMemoryMap();
    if (EFI_ERROR(status))
    {
        printf("Could not retrieve memory map\n");
        return status;
    }

    putchar('\n');
    g_memoryMap.Print();
    putchar('\n');
    g_modules.Print();

    if (LoadLauncher() != 0)
    {
        printf("Failed to load Launcher\n");
        return EFI_LOAD_ERROR;
    }

    // ExitBootServices(hImage);


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



extern "C" efi::status_t EFIAPI efi_main(efi::handle_t hImage, efi::SystemTable* systemTable)
{
    if (!systemTable)
        return EFI_INVALID_PARAMETER;

    g_efiImage = hImage;
    g_efiSystemTable = systemTable;
    g_efiBootServices = systemTable->bootServices;
    g_efiRuntimeServices = systemTable->runtimeServices;

    InitConsole();

    CallGlobalConstructors();

    printf("Rainbow EFI Bootloader (" STRINGIZE(ARCH) ")\n\n", (int)sizeof(void*)*8);

    efi::status_t status = Boot();

    if (EFI_ERROR(status))
    {
        printf("Boot() returned error %p\n", (void*)status);
    }

    getchar();

    CallGlobalDestructors();

    return status;
}
