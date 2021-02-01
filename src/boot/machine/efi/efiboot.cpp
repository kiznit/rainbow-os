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
#include <cstdlib>
#include <cstring>
#include <string>
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
        MemoryFlags flags;

        switch (descriptor->Type)
        {

        case EfiLoaderCode:
        case EfiBootServicesCode:
            type = MemoryType::Bootloader;
            flags = MemoryFlags::Code;
            break;

        case EfiLoaderData:
        case EfiBootServicesData:
            type = MemoryType::Bootloader;
            flags = MemoryFlags::None;
            break;

        case EfiRuntimeServicesCode:
            type = MemoryType::Firmware;
            flags = MemoryFlags::Code;
            break;

        case EfiRuntimeServicesData:
            type = MemoryType::Firmware;
            flags = MemoryFlags::None;
            break;

        case EfiConventionalMemory:
            type = MemoryType::Available;
            flags = MemoryFlags::None;
            break;

        case EfiUnusableMemory:
            type = MemoryType::Unusable;
            flags = MemoryFlags::None;
            break;

        case EfiACPIReclaimMemory:
            type = MemoryType::AcpiReclaimable;
            flags = MemoryFlags::None;
            break;

        case EfiACPIMemoryNVS:
            type = MemoryType::AcpiNvs;
            flags = MemoryFlags::None;
            break;

        case EfiPersistentMemory:
            type = MemoryType::Persistent;
            flags = MemoryFlags::None;
            break;

        case EfiReservedMemoryType:
        case EfiMemoryMappedIO:
        case EfiMemoryMappedIOPortSpace:
        case EfiPalCode:
        default:
            type = MemoryType::Reserved;
            flags = MemoryFlags::None;
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
    m_fileSystem(hImage, systemTable->BootServices)
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
    std::vector<EFI_HANDLE> handles;
    EFI_STATUS status;

    // LocateHandle() should only be called twice... But I don't want to write it twice :)
    while ((status = m_bootServices->LocateHandle(ByProtocol, &g_efiGraphicsOutputProtocolGuid, nullptr, &size, handles.data())) == EFI_BUFFER_TOO_SMALL)
    {
        handles.resize(size / sizeof(EFI_HANDLE));
    }

    if (EFI_ERROR(status))
    {
        Fatal("Failed to retrieve retrieve EFI display handles\n");
    }

    for (auto handle: handles)
    {
        EFI_DEVICE_PATH_PROTOCOL* dpp = nullptr;
        m_bootServices->HandleProtocol(handle, &g_efiDevicePathProtocolGuid, (void**)&dpp);
        // If dpp is NULL, this is the "Console Splitter" driver. It is used to draw on all
        // screens at the same time and doesn't represent a real hardware device.
        if (!dpp) continue;

        EFI_GRAPHICS_OUTPUT_PROTOCOL* gop = nullptr;
        m_bootServices->HandleProtocol(handle, &g_efiGraphicsOutputProtocolGuid, (void**)&gop);
        // gop is not expected to be null, but let's play safe.
        if (!gop) continue;

        EFI_EDID_ACTIVE_PROTOCOL* edid = nullptr;
        if (EFI_ERROR(m_bootServices->HandleProtocol(handle, &g_efiEdidActiveProtocolGuid, (void**)&edid)) || !edid)
        {
            m_bootServices->HandleProtocol(handle, &g_efiEdidDiscoveredProtocolGuid, (void**)&edid);
        }

        m_displays.push_back(std::move(EfiDisplay(gop, edid)));
    }
}


void* EfiBoot::AllocatePages(int pageCount, physaddr_t maxAddress)
{
    maxAddress = std::min(maxAddress, MAX_ALLOC_ADDRESS);

    EFI_PHYSICAL_ADDRESS memory = maxAddress - 1;
    EFI_STATUS status = m_bootServices->AllocatePages(AllocateMaxAddress, EfiLoaderData, pageCount, &memory);
    if (EFI_ERROR(status))
    {
        throw std::bad_alloc();
    }

    return (void*)memory;
}


void EfiBoot::Exit(MemoryMap& memoryMap)
{
    UINTN size = 0;
    EFI_MEMORY_DESCRIPTOR* descriptors = nullptr;
    UINTN memoryMapKey = 0;
    UINTN descriptorSize = 0;
    UINT32 descriptorVersion = 0;

    // 1) Retrieve the memory map from the firmware
    EFI_STATUS status;

    std::vector<char> buffer;
    while ((status = m_bootServices->GetMemoryMap(&size, descriptors, &memoryMapKey, &descriptorSize, &descriptorVersion)) == EFI_BUFFER_TOO_SMALL)
    {
        // Extra space to try to prevent "partial shutdown" when calling ExitBootServices().
        // See comment below about what a "partial shutdown" is.
        size += descriptorSize * 10;

        buffer.resize(size);
        descriptors = (EFI_MEMORY_DESCRIPTOR*)buffer.data();
    }

    // 2) Exit boot services - it is possible for the firmware to modify the memory map
    // during a call to ExitBootServices(). A so-called "partial shutdown".
    // When that happens, ExitBootServices() will return EFI_INVALID_PARAMETER.
    while ((status = m_bootServices->ExitBootServices(m_hImage, memoryMapKey)) == EFI_INVALID_PARAMETER)
    {
        // Memory map changed during ExitBootServices(), the only APIs we are allowed to
        // call at this point are GetMemoryMap() and ExitBootServices().
        size = buffer.size(); // Probbaly not needed, but let's play safe since EFI could change that value behind our back (you never know!)
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
    const Acpi::Rsdp* result = nullptr;

    for (unsigned int i = 0; i != m_systemTable->NumberOfTableEntries; ++i)
    {
        const EFI_CONFIGURATION_TABLE& table = m_systemTable->ConfigurationTable[i];

        // ACPI 1.0
        if (table.VendorGuid == g_efiAcpi1TableGuid)
        {
            const auto rsdp = (Acpi::Rsdp*)table.VendorTable;
            if (rsdp && rsdp->VerifyChecksum())
            {
                result = rsdp;
            }
            // Continue looking for ACPI 2.0 table
        }

        // ACPI 2.0
        if (table.VendorGuid == g_efiAcpi2TableGuid)
        {
            const auto rsdp = (Acpi::Rsdp20*)table.VendorTable;
            if (rsdp && rsdp->VerifyExtendedChecksum())
            {
                result = rsdp;
                break;
            }
        }
    }

    return result;
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
    return m_displays.size();
}


IDisplay* EfiBoot::GetDisplay(int index) const
{
    if (index < 0 || index >= (int)m_displays.size())
    {
        return nullptr;
    }

    return const_cast<EfiDisplay*>(&m_displays[index]);
}


bool EfiBoot::LoadModule(const char* name, Module& module) const
{
    std::u16string filename(u"\\EFI\\rainbow\\");
    filename.append(name, name + strlen(name));

    void* data;
    size_t size;
    if (!m_fileSystem.ReadFile(filename.c_str(), &data, &size))
    {
        return false;
    }

    module.address = (uintptr_t)data;
    module.size = size;

    return true;
}


void EfiBoot::Print(const char* string, size_t length)
{
    const auto console = m_systemTable->ConOut;
    if (!console) return;

    // Convert string to wide chars as required by UEFI APIs
    std::u16string u16string;
    u16string.reserve(length + 2);
    for (const char* p = string; length--; ++p)
    {
        if (*p == '\n')
        {
            u16string.push_back('\r');
        }

        u16string.push_back(*p);
    }

    console->OutputString(console, const_cast<char16_t*>(u16string.c_str()));
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

    Log("Rainbow UEFI Bootloader (" STRINGIZE(KERNEL_ARCH) ")\n\n");

    Boot(&efiBoot);

    return EFI_SUCCESS;
}