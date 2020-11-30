/*
    Copyright (c) 2020, Thierry Tremblay
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

#include "efiboot.hpp"
#include <metal/crt.hpp>
#include "efidisplay.hpp"
#include "memory.hpp"

// Intel's UEFI header do not properly define EFI_MEMORY_DESCRIPTOR for GCC.
// This check ensures that it is.
static_assert(offsetof(EFI_MEMORY_DESCRIPTOR, PhysicalStart) == 8);

static EFI_GUID g_efiDevicePathProtocolGuid = EFI_DEVICE_PATH_PROTOCOL_GUID;
static EFI_GUID g_efiEdidActiveProtocolGuid = EFI_EDID_ACTIVE_PROTOCOL_GUID;
static EFI_GUID g_efiEdidDiscoveredProtocolGuid = EFI_EDID_DISCOVERED_PROTOCOL_GUID;
static EFI_GUID g_efiGraphicsOutputProtocolGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;

static EFI_GUID g_efiAcpi1TableGuid = { 0xeb9d2d30, 0x2d88, 0x11d3, { 0x9a, 0x16, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d } };
static EFI_GUID g_efiAcpi2TableGuid = { 0x8868e871, 0xe4f1, 0x11d3, { 0xbc, 0x22, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81 } };

static inline bool operator==(const EFI_GUID& a, const EFI_GUID& b)
{
    return 0 == memcmp(&a, &b, sizeof(a));
}


// Convert EFI memory map to our own format
static void BuildMemoryMap(MemoryMap& memoryMap, const EFI_MEMORY_DESCRIPTOR* descriptors, size_t descriptorCount, size_t descriptorSize)
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

        memoryMap.AddBytes(type, flags, descriptor->PhysicalStart, descriptor->NumberOfPages * EFI_PAGE_SIZE);
    }
}


EfiBoot::EfiBoot(EFI_HANDLE hImage, EFI_SYSTEM_TABLE* systemTable)
:   m_hImage(hImage),
    m_systemTable(systemTable),
    m_bootServices(systemTable->BootServices),
    m_runtimeServices(systemTable->RuntimeServices),
    m_fileSystem(hImage, systemTable->BootServices),
    m_displayCount(0),
    m_displays(nullptr)
{
    // We do this so that the CRT can be used right away.
    g_bootServices = this;

    InitConsole();
    InitDisplays();
}


void EfiBoot::InitConsole()
{
    const auto console = m_systemTable->ConOut;

    // Mode 0 is always 80x25 text mode and is always supported
    // Mode 1 is always 80x50 text mode and isn't always supported
    // Modes 2+ are differents on every device
    size_t mode = 0;
    size_t width = 80;
    size_t height = 25;

    // Find the highest width x height mode available
    for (size_t m = 1; ; ++m)
    {
        size_t  w, h;
        auto status = console->QueryMode(console, m, &w, &h);
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


void EfiBoot::InitDisplays()
{
    UINTN size = 0;
    EFI_HANDLE* handles = nullptr;
    EFI_STATUS status;

    // LocateHandle() should only be called twice... But I don't want to write it twice :)
    while ((status = m_bootServices->LocateHandle(ByProtocol, &g_efiGraphicsOutputProtocolGuid, nullptr, &size, handles)) == EFI_BUFFER_TOO_SMALL)
    {
        handles = (EFI_HANDLE*)realloc(handles, size);
        if (!handles)
        {
            Fatal("Failed to allocate memory to retrieve EFI display handles\n");
        }
    }

    if (EFI_ERROR(status))
    {
        Fatal("Failed to retrieve retrieve EFI display handles\n");
    }

    const int count = size / sizeof(EFI_HANDLE);

    m_displayCount = 0;
    m_displays = (EfiDisplay*) malloc(count * sizeof(EfiDisplay));

    for (int i = 0; i != count; ++i)
    {
        EFI_DEVICE_PATH_PROTOCOL* dpp = nullptr;
        m_bootServices->HandleProtocol(handles[i], &g_efiDevicePathProtocolGuid, (void**)&dpp);
        // If dpp is NULL, this is the "Console Splitter" driver. It is used to draw on all
        // screens at the same time and doesn't represent a real hardware device.
        if (!dpp) continue;

        EFI_GRAPHICS_OUTPUT_PROTOCOL* gop = nullptr;
        m_bootServices->HandleProtocol(handles[i], &g_efiGraphicsOutputProtocolGuid, (void**)&gop);
        // gop is not expected to be null, but let's play safe.
        if (!gop) continue;

        EFI_EDID_ACTIVE_PROTOCOL* edid = nullptr;
        if (EFI_ERROR(m_bootServices->HandleProtocol(handles[i], &g_efiEdidActiveProtocolGuid, (void**)&edid)) || !edid)
        {
            m_bootServices->HandleProtocol(handles[i], &g_efiEdidDiscoveredProtocolGuid, (void**)&edid);
        }

        new (&m_displays[m_displayCount]) EfiDisplay(gop, edid);
        ++m_displayCount;
    }

    free(handles);
}


void* EfiBoot::AllocatePages(int pageCount, physaddr_t maxAddress)
{
    EFI_PHYSICAL_ADDRESS memory = maxAddress - 1;
    EFI_STATUS status = m_bootServices->AllocatePages(AllocateMaxAddress, EfiLoaderData, pageCount, &memory);
    if (EFI_ERROR(status))
    {
        Fatal("EFI failed to allocate %d memory pages: %p\n", pageCount, status);
    }

    return (void*)memory;
}


void EfiBoot::Exit(MemoryMap& memoryMap)
{
    UINTN size = 0;
    UINTN allocatedSize = 0;
    EFI_MEMORY_DESCRIPTOR* descriptors = nullptr;
    UINTN memoryMapKey = 0;
    UINTN descriptorSize = 0;
    UINT32 descriptorVersion = 0;

    // 1) Retrieve the memory map from the firmware
    EFI_STATUS status;
    while ((status = m_bootServices->GetMemoryMap(&size, descriptors, &memoryMapKey, &descriptorSize, &descriptorVersion)) == EFI_BUFFER_TOO_SMALL)
    {
        // Extra space to try to prevent "partial shutdown" when calling ExitBootServices().
        // See comment below about what a "partial shutdown" is.
        size += descriptorSize * 10;

        descriptors = (EFI_MEMORY_DESCRIPTOR*)realloc(descriptors, size);
        if (!descriptors)
        {
            Fatal("Failed to allocate memory to retrieve the EFI memory map\n");
        }

        allocatedSize = size;
    }

    // 2) Exit boot services - it is possible for the firmware to modify the memory map
    // during a call to ExitBootServices(). A so-called "partial shutdown".
    // When that happens, ExitBootServices() will return EFI_INVALID_PARAMETER.
    while ((status = m_bootServices->ExitBootServices(m_hImage, memoryMapKey)) == EFI_INVALID_PARAMETER)
    {
        // Memory map changed during ExitBootServices(), the only APIs we are allowed to
        // call at this point are GetMemoryMap() and ExitBootServices().
        size = allocatedSize;
        status = m_bootServices->GetMemoryMap(&size, descriptors, &memoryMapKey, &descriptorSize, &descriptorVersion);
        if (EFI_ERROR(status))
        {
            break;
        }
    }

    if (EFI_ERROR(status))
    {
        Fatal("Failed to retrieve the EFI memory map: %p\n", status);
    }

    // Clear out fields we can't use anymore
    m_systemTable->ConsoleInHandle = 0;
    m_systemTable->ConIn = nullptr;
    m_systemTable->ConsoleOutHandle = 0;
    m_systemTable->ConOut = nullptr;
    m_systemTable->StandardErrorHandle = 0;
    m_systemTable->StdErr = nullptr;
    m_systemTable->BootServices = nullptr;

    m_bootServices = nullptr;

    BuildMemoryMap(memoryMap, descriptors, size / descriptorSize, descriptorSize);
}


const Acpi::Rsdp* EfiBoot::FindAcpiRsdp() const
{
    Acpi::Rsdp* rsdp = nullptr;

    for (unsigned int i = 0; i != m_systemTable->NumberOfTableEntries; ++i)
    {
        const EFI_CONFIGURATION_TABLE& table = m_systemTable->ConfigurationTable[i];

        if (table.VendorGuid == g_efiAcpi2TableGuid)
        {
            rsdp = (Acpi::Rsdp*)table.VendorTable;
            break;
        }

        if (table.VendorGuid == g_efiAcpi1TableGuid)
        {
            rsdp = (Acpi::Rsdp*)table.VendorTable;
            // Continue looking for ACPI 2.0 table
        }
    }

    return rsdp;
}


int EfiBoot::GetChar()
{
    const auto console = m_systemTable->ConIn;

    for (;;)
    {
        UINTN index;
        EFI_STATUS status = m_bootServices->WaitForEvent(1, &console->WaitForKey, &index);
        if (EFI_ERROR(status))
        {
            return -1;
        }

        EFI_INPUT_KEY key;
        status = console->ReadKeyStroke(console, &key);
        if (EFI_ERROR(status))
        {
            if (status == EFI_NOT_READY)
            {
                continue;
            }

            return 1;
        }

        return key.UnicodeChar;
    }
}


int EfiBoot::GetDisplayCount() const
{
    return m_displayCount;
}


IDisplay* EfiBoot::GetDisplay(int index) const
{
    if (index < 0 || index >= m_displayCount)
    {
        return nullptr;
    }

    return &m_displays[index];
}


bool EfiBoot::LoadModule(const char* name, Module& module) const
{
    static const CHAR16 prefix[14] = L"\\EFI\\rainbow\\";

    const auto length = strlen(name);
    const auto wideName = (CHAR16*)alloca((length + 1) * sizeof(CHAR16) + 13);

    memcpy(wideName, prefix, sizeof(prefix));

    for (size_t i = 0; i != length; ++i)
    {
        wideName[i + 13] = (unsigned char) name[i];
    }

    wideName[length + 13] = '\0';

    void* data;
    size_t size;
    if (!m_fileSystem.ReadFile(wideName, &data, &size))
    {
        return false;
    }

    module.address = (uintptr_t)data;
    module.size = size;

    return true;
}


void EfiBoot::Print(const char* string)
{
    const auto console = m_systemTable->ConOut;
    if (!console) return;

    CHAR16 buffer[500];
    size_t count = 0;

    for (const char* p = string; *p; ++p)
    {
        const unsigned char c = *p;

        if (c == '\n')
        {
            buffer[count++] = '\r';
        }

        buffer[count++] = c;

        if (count >= ARRAY_LENGTH(buffer) - 3)
        {
            buffer[count] = '\0';
            console->OutputString(console, buffer);
            count = 0;
        }
    }

    if (count > 0)
    {
        buffer[count] = '\0';
        console->OutputString(console, buffer);
    }
}


void EfiBoot::Reboot()
{
    m_runtimeServices->ResetSystem(EfiResetWarm, EFI_SUCCESS, 0, nullptr);

    // If that didn't work, cause a triple fault
    // For now, cause a triple fault
    asm volatile ("int $3");

    // Play safe, don't assume the above will actually work
    for (;;);
}



extern "C" EFI_STATUS efi_main(EFI_HANDLE hImage, EFI_SYSTEM_TABLE* systemTable)
{
    EfiBoot efiBoot(hImage, systemTable);

    g_bootServices->Print("Rainbow UEFI Bootloader (" STRINGIZE(KERNEL_ARCH) ")\n\n");

    Boot(&efiBoot);

    return EFI_SUCCESS;
}
