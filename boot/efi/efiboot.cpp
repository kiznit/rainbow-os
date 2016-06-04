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

#include <Uefi.h>
#include <Guid/FileInfo.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>

#include "boot.hpp"
#include "elf.hpp"
#include "memory.hpp"


// Sanity checks
static_assert(sizeof(wchar_t) == 2, "wchar_t must be 2 bytes wide");


// Globals
EFI_HANDLE              g_efiImage;
EFI_SYSTEM_TABLE*       g_efiSystemTable;
EFI_BOOT_SERVICES*      g_efiBootServices;
EFI_RUNTIME_SERVICES*   g_efiRuntimeServices;

static MemoryMap g_memoryMap;
static BootInfo g_bootInfo;

static EFI_GUID g_efiLoadedImageProtocolGuid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
static EFI_GUID g_efiSimpleFileSystemProtocolGuid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
static EFI_GUID g_EfiFileInfoGuid = EFI_FILE_INFO_ID;



// Smart file pointer
class scoped_file_ptr
{
public:
    scoped_file_ptr(EFI_FILE_PROTOCOL* file = NULL) : m_file(file) {}
    ~scoped_file_ptr()                              { close(); }

    void close()                                    { if (m_file) { m_file->Close(m_file); m_file = NULL; } }
    void reset(EFI_FILE_PROTOCOL* file = NULL)      { m_file = file; }
    EFI_FILE_PROTOCOL*& raw()                       { return m_file; }
    EFI_FILE_PROTOCOL* operator->() const           { return m_file; }
    EFI_FILE_PROTOCOL* get() const                  { return m_file; }

private:
    EFI_FILE_PROTOCOL* m_file;
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



static EFI_STATUS BuildMemoryMap(UINTN* mapKey)
{
    // Retrieve the memory map from EFI
    UINTN descriptorCount = 0;
    UINTN descriptorSize = 0;
    UINT32 descriptorVersion = 0;
    *mapKey = 0;

    EFI_MEMORY_DESCRIPTOR* memoryMap = NULL;
    UINTN size = 0;

    EFI_STATUS status = EFI_BUFFER_TOO_SMALL;
    while (status == EFI_BUFFER_TOO_SMALL)
    {
        free(memoryMap);
        memoryMap = NULL;

        memoryMap = (EFI_MEMORY_DESCRIPTOR*) malloc(size);
        status = g_efiBootServices->GetMemoryMap(&size, memoryMap, mapKey, &descriptorSize, &descriptorVersion);
    }

    if (EFI_ERROR(status))
    {
        free(memoryMap);
        return status;
    }

    descriptorCount = size / descriptorSize;


    // Convert EFI memory map to our own format
    EFI_MEMORY_DESCRIPTOR* descriptor = memoryMap;
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

    return EFI_SUCCESS;
}



static EFI_STATUS ExitBootServices()
{
    UINTN key;
    EFI_STATUS status = BuildMemoryMap(&key);
    if (EFI_ERROR(status))
    {
        printf("Failed to build memory map: %p\n", (void*)status);
        return status;
    }

    //g_memoryMap.Sanitize();
    //g_memoryMap.Print();

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



static void Boot32(uint32_t kernelVirtualAddress, uint32_t entry, void* kernel, size_t kernelSize)
{
    printf("Boot32(%08x, %p, %lu)\n", (unsigned int)entry, kernel, kernelSize);

    // Initialize paging (PAE)
    //      PML3: 0x4 entries (PDPT)
    //      PML2: 0x800 entries (Page Directories)
    //      PML1: 0x100000 entries (Page Tables)

    // 1) Identity map the first 4GB of physical memory
    const physaddr_t pdpt = g_memoryMap.AllocatePages(MemoryType_Bootloader, 1);
    const physaddr_t pageDirectories = g_memoryMap.AllocatePages(MemoryType_Bootloader, 4);

    // PDPT
    physaddr_t* const pml3 = (physaddr_t*)pdpt;

    pml3[0] = (pageDirectories)                        | PAGE_PRESENT;
    pml3[1] = (pageDirectories + MEMORY_PAGE_SIZE)     | PAGE_PRESENT;
    pml3[2] = (pageDirectories + MEMORY_PAGE_SIZE * 2) | PAGE_PRESENT;
    pml3[3] = (pageDirectories + MEMORY_PAGE_SIZE * 3) | PAGE_PRESENT;

    // Page directories
    physaddr_t* const pml2 = (physaddr_t*)pageDirectories;

    physaddr_t address = 0;
    for (int i = 0; i != 0x800; ++i, address += 2*1024*1024)
    {
        pml2[i] = address | PAGE_LARGE | PAGE_PRESENT;
    }

    // 2) Map the kernel
    const physaddr_t kernel_physical_start = (uintptr_t)kernel;
    const physaddr_t kernel_virtual_start = kernelVirtualAddress;
    const physaddr_t kernel_virtual_end = kernel_virtual_start + kernelSize;
    const physaddr_t kernel_virtual_offset = kernel_virtual_start - kernel_physical_start;

    printf("kernel: %016llx, %016llx, %016llx\n", kernel_virtual_start, kernel_virtual_end, kernel_virtual_offset);

    physaddr_t pml2_start = (kernel_virtual_start >> 21);
    physaddr_t pml2_end   = (kernel_virtual_end >> 21);
    printf("  pml2: %016llx - %016llx\n", pml2_start, pml2_end);

    const int pml2_count = (pml2_end - pml2_start) + 1;

    const physaddr_t pageTables = g_memoryMap.AllocatePages(MemoryType_Bootloader, pml2_count);

    printf("Allocated %d pml2 pages for pml1 at %016llx\n", (int)pml2_count, pageTables);

    //printf("pageTables : %016llx\n", pageTables - pml1_start * 8);

    address = pageTables;
    for (physaddr_t i = pml2_start; i <= pml2_end; ++i, address += MEMORY_PAGE_SIZE)
    {
        printf("pml2[0x%x]: %016llx", (int)i, pml2[i]);
        pml2[i] = address | PAGE_PRESENT;
        printf("--> %016llx\n", pml2[i]);
    }

    physaddr_t* const pml1 = (physaddr_t*)pageTables;

    address = pml2_start << 21;
    for (int i = 0; i != pml2_count * 512; ++i, address += MEMORY_PAGE_SIZE)
    {
        if (address >= kernel_virtual_start && address < kernel_virtual_end)
        {
            pml1[i] = (address - kernel_virtual_offset) | PAGE_WRITE | PAGE_PRESENT;
        }
        else
        {
            pml1[i] = address | PAGE_PRESENT;
        }
    }



    printf("Calling StartKernel32()\n");

    StartKernel32(&g_bootInfo, pdpt, entry);

    printf("kernel_main() returned!\n");
}



static void Boot64(uint64_t kernelVirtualAddress, physaddr_t entry, void* kernel, size_t kernelSize)
{
    printf("Boot64(%016llx, %p, %lu)\n", entry, kernel, kernelSize);

    // Initialize paging
    //      PML4: 0x200 entries
    //      PML3: 0x40000 entries (PDPTs)
    //      PML2: 0x8000000 entries (Page Directories)
    //      PML1: 0x1000000000 entries (Page Tables)

    // 1) Identity map the first 4GB of physical memory
    const physaddr_t PML4 = g_memoryMap.AllocatePages(MemoryType_Bootloader, 1);
    const physaddr_t pdpt = g_memoryMap.AllocatePages(MemoryType_Bootloader, 2);
    const physaddr_t pageDirectories = g_memoryMap.AllocatePages(MemoryType_Bootloader, 5);

    physaddr_t* const pml4 = (physaddr_t*)PML4;
    memset(pml4, 0, MEMORY_PAGE_SIZE);
    pml4[0] = pdpt | PAGE_PRESENT;

    printf("cr3 (pml4)      : %016llx\n", PML4);
    printf("pdpt            : %016llx\n", pdpt);
    printf("pageDirectories : %016llx\n", pageDirectories);

    // PDPT
    physaddr_t* pml3 = (physaddr_t*)pdpt;
    memset(pml3, 0, MEMORY_PAGE_SIZE);

    pml3[0] = (pageDirectories)                        | PAGE_PRESENT;
    pml3[1] = (pageDirectories + MEMORY_PAGE_SIZE)     | PAGE_PRESENT;
    pml3[2] = (pageDirectories + MEMORY_PAGE_SIZE * 2) | PAGE_PRESENT;
    pml3[3] = (pageDirectories + MEMORY_PAGE_SIZE * 3) | PAGE_PRESENT;

    // Page directories
    physaddr_t* pml2 = (physaddr_t*)pageDirectories;

    physaddr_t address = 0;
    for (int i = 0; i != 0x800; ++i, address += 2*1024*1024)
    {
        pml2[i] = address | PAGE_LARGE | PAGE_PRESENT;
    }



    // 2) Map the kernel
    const physaddr_t kernel_physical_start = (uintptr_t)kernel;
    const physaddr_t kernel_virtual_start = kernelVirtualAddress;
    const physaddr_t kernel_virtual_end = kernel_virtual_start + kernelSize;
    const physaddr_t kernel_virtual_offset = kernel_virtual_start - kernel_physical_start;

    printf("kernel: %016llx, %016llx, %016llx\n", kernel_virtual_start, kernel_virtual_end, kernel_virtual_offset);

    // PDPT
    physaddr_t pml4_start = (kernel_virtual_start >> 39) & 0x1FF;
    physaddr_t pml4_end   = (kernel_virtual_end >> 39) & 0x1FF;
    printf("  pml4: %016llx - %016llx\n", pml4_start, pml4_end);

    physaddr_t pml3_start = (kernel_virtual_start >> 30) & 0x3FFFF;
    physaddr_t pml3_end   = (kernel_virtual_end >> 30) & 0x3FFFF;
    printf("  pml3: %016llx - %016llx\n", pml3_start, pml3_end);

    physaddr_t pml2_start = (kernel_virtual_start >> 21) & 0x7FFFFFF;
    physaddr_t pml2_end   = (kernel_virtual_end >> 21) & 0x7FFFFFF;
    printf("  pml2: %016llx - %016llx\n", pml2_start, pml2_end);

    physaddr_t pml1_start = (kernel_virtual_start >> 12) & 0xFFFFFFFFFull;
    physaddr_t pml1_end   = (kernel_virtual_end >> 12) & 0xFFFFFFFFF;
    printf("  pml1: %016llx - %016llx\n", pml1_start, pml1_end);

    pml4[511] = (pdpt + MEMORY_PAGE_SIZE) | PAGE_PRESENT;

    pml3 = (physaddr_t*)(pdpt + MEMORY_PAGE_SIZE);
    memset(pml3, 0, MEMORY_PAGE_SIZE);

    pml3[0x1ff] = (pageDirectories + MEMORY_PAGE_SIZE * 4) | PAGE_PRESENT;

    pml2 = (physaddr_t*)(pageDirectories + MEMORY_PAGE_SIZE * 4);
    memset(pml2, 0, MEMORY_PAGE_SIZE);


    const int pml2_count = (pml2_end - pml2_start) + 1;

    const physaddr_t pageTables = g_memoryMap.AllocatePages(MemoryType_Bootloader, pml2_count);

    printf("Allocated %d pml2 pages for pml1 (page tables) at %016llx\n", (int)pml2_count, pageTables);

    address = pageTables;
    for (physaddr_t i = pml2_start; i <= pml2_end; ++i, address += MEMORY_PAGE_SIZE)
    {
        printf("pml2[0x%x]: %016llx", (int)(i & 0x1FF), pml2[i & 0x1FF]);
        pml2[i & 0x1FF] = address | PAGE_PRESENT;
        printf(" --> %016llx\n", pml2[i & 0x1FF]);
    }

    physaddr_t* const pml1 = (physaddr_t*)pageTables;

    address = (kernel_virtual_start >> 21) << 21;
    for (int i = 0; i != pml2_count * 512; ++i, address += MEMORY_PAGE_SIZE)
    {
        if (address >= kernel_virtual_start && address < kernel_virtual_end)
        {
            pml1[i] = (address - kernel_virtual_offset) | PAGE_WRITE | PAGE_PRESENT;
            //printf("pml1[%d] (%p): address %016llx --> %016llx\n", i, &pml1[i], address, pml1[i]);
        }
        else
        {
            pml1[i] = 0;
        }
    }


    printf("g_booInfo address: %p\n", &g_bootInfo);

    printf("Sanity check:\n");
    printf("Boot64(%016llx, %p, %lu)\n", entry, kernel, kernelSize);

    for (int i4 = 0; i4 != 512; ++i4)
    {
        if (pml4[i4] != 0)
        {
            printf("    pml4[%x] = %016llx\n", i4, pml4[i4]);

            const physaddr_t* pml3 = (physaddr_t*)(pml4[i4] & ~0xfff);
            for (int i3 = 0; i3 != 512; ++i3)
            {
                if (pml3[i3] != 0)
                {
                    printf("        pml3[%x] = %016llx\n", i3, pml3[i3]);

                    const physaddr_t* pml2 = (physaddr_t*)(pml3[i3] & ~0xfff);
                    for (int i2 = 0; i2 != 512; ++i2)
                    {
                        if (i4 == 0)
                            continue;

                        if (pml2[i2] != 0)
                        {
                            printf("          pml2[%x] = %016llx\n", i2, pml2[i2]);

                            const physaddr_t* pml1 = (physaddr_t*)(pml2[i2] & ~0xfff);
                            for (int i1 = 0; i1 != 512; ++i1)
                            {
                                //if (i1 != 0 || i2 >3)
                                //   continue;

                                if (pml1[i1] != 0)
                                {
                                    printf("            pml1[%x] @ %p = %016llx\n", i1, &pml1[i1], pml1[i1]);
                                }
                            }
                        }
                    }
                }
            }
        }
    }


    printf("Calling StartKernel64()\n");

    StartKernel64(&g_bootInfo, PML4, entry);

    printf("kernel_main() returned!\n");
}



static EFI_STATUS LoadAndExecuteKernel(EFI_FILE_PROTOCOL* fileSystemRoot, const wchar_t* path)
{
    EFI_STATUS status;

    // Open file
    scoped_file_ptr file;
    status = fileSystemRoot->Open(fileSystemRoot, &file.raw(), (CHAR16*)path, EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(status))
        return status;

    // Retrieve file info
    UINTN infoSize = 0;
    status = file->GetInfo(file.get(), &g_EfiFileInfoGuid, &infoSize, NULL);
    if (EFI_ERROR(status) && status != EFI_BUFFER_TOO_SMALL)
        return status;

    MemoryBuffer fileInfoBuffer(infoSize);
    if (!fileInfoBuffer.Valid())
        return EFI_OUT_OF_RESOURCES;

    status = file->GetInfo(file.get(), &g_EfiFileInfoGuid, &infoSize, fileInfoBuffer.begin());
    if (EFI_ERROR(status))
        return status;

    EFI_FILE_INFO* info = (EFI_FILE_INFO*)fileInfoBuffer.begin();

    // Read file
    MemoryBuffer fileBuffer(info->FileSize);
    if (!fileBuffer.Valid())
        return EFI_OUT_OF_RESOURCES;

    size_t readSize = info->FileSize;

    status = file->Read(file.get(), &readSize, fileBuffer.begin());
    if (EFI_ERROR(status) || readSize != info->FileSize)
        return status;

    // Read ELF
    ElfLoader elf(fileBuffer.begin(), readSize);
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

    const unsigned int elfSize = elf.GetMemorySize();
    const unsigned int elfAlignment = elf.GetMemoryAlignment();

    void* memory = NULL;

    if (elfAlignment <= EFI_PAGE_SIZE)
    {
        physaddr_t address = 0;
        status = g_efiBootServices->AllocatePages(AllocateAnyPages, EfiLoaderData, EFI_SIZE_TO_PAGES(elfSize), &address);
        if (EFI_ERROR(status))
            return status;

        memory = (void*)address;
    }

    if (!memory)
    {
        printf("Could not allocate memory to load launcher (size: %u, alignment: %u)\n", elfSize, elfAlignment);
        return EFI_OUT_OF_RESOURCES;
    }

    printf("Launcher memory allocated at %p\n", memory);

    physaddr_t entry = elf.Load(memory);
    if (entry == 0)
    {
        printf("Error loading launcher\n");
        return EFI_UNSUPPORTED;
    }

    printf("launcher_main() at %p\n", entry);

    // Get ready to execute kernel
    status = ExitBootServices();
    if (EFI_ERROR(status))
    {
        return status;
    }


    g_memoryMap.Sanitize();
    g_memoryMap.Print();

    g_bootInfo.memoryDescriptorCount = g_memoryMap.size();
    g_bootInfo.memoryDescriptors = (uintptr_t)g_memoryMap.begin();


    // Execute kernel
    if (elf.Is32Bits())
    {
        Boot32(elf.GetStartAddress(), entry, memory, elfSize);
    }
    else
    {
        Boot64(elf.GetStartAddress(), entry, memory, elfSize);
    }

    // We don't expect launcher to return... If it does, return an error.
    return EFI_UNSUPPORTED;
}



static EFI_STATUS Boot()
{
    EFI_STATUS status;
    EFI_LOADED_IMAGE_PROTOCOL* image = NULL;

    status = g_efiBootServices->HandleProtocol(g_efiImage, &g_efiLoadedImageProtocolGuid, (void**)&image);
    if (EFI_ERROR(status))
    {
        printf("Could not open EfiLoadedImageProtocol\n");
        return status;
    }

    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs;
    status = g_efiBootServices->HandleProtocol(image->DeviceHandle, &g_efiSimpleFileSystemProtocolGuid, (void**)&fs);
    if (EFI_ERROR(status))
        return status;

    scoped_file_ptr fileSystemRoot;
    status = fs->OpenVolume(fs, &fileSystemRoot.raw());
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
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* output = g_efiSystemTable->ConOut;

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
        EFI_STATUS status = output->QueryMode(output, m, &w, &h);
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
    output->EnableCursor(output, FALSE);
    output->SetCursorPosition(output, 0, 0);
}



static void Initialize(EFI_HANDLE hImage, EFI_SYSTEM_TABLE* systemTable)
{
    g_efiImage = hImage;
    g_efiSystemTable = systemTable;
    g_efiBootServices = systemTable->BootServices;
    g_efiRuntimeServices = systemTable->RuntimeServices;

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



extern "C" EFI_STATUS EFIAPI efi_main(EFI_HANDLE hImage, EFI_SYSTEM_TABLE* systemTable)
{
    if (!hImage || !systemTable)
        return EFI_INVALID_PARAMETER;

    Initialize(hImage, systemTable);

    g_bootInfo.version = RAINBOW_BOOT_VERSION;
    g_bootInfo.firmware = Firmware_EFI;

    // Welcome message
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* output = g_efiSystemTable->ConOut;
    if (output)
    {
        const int32_t backupAttributes = output->Mode->Attribute;

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


    EFI_STATUS status = Boot();

    printf("Boot() returned with status %p\n", (void*)status);

    Shutdown();

    return status;
}
