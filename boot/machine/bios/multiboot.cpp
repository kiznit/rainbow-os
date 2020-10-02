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

#include "multiboot.hpp"
#include <multiboot/multiboot.h>
#include <multiboot/multiboot2.h>
#include <string.h>
#include "bios.hpp"
#include "memory.hpp"
#include "graphics/surface.hpp"

extern MemoryMap g_memoryMap;


/*
    Multiboot structures missing from headers
*/

struct multiboot_module
{
    uint32_t mod_start;
    uint32_t mod_end;
    const char* string;
    uint32_t reserved;
};


struct multiboot2_info
{
    uint32_t total_size;
    uint32_t reserved;
};


struct multiboot2_module
{
    multiboot2_header_tag tag;
    uint32_t mod_start;
    uint32_t mod_end;
    char     string[];
};


Multiboot::Multiboot(unsigned int magic, const void* mbi)
{
    // We do this so that the CRT can be used right away.
    g_bootServices = this;

    // 0x00000000 - 0x000003FF - Interrupt Vector Table
    // 0x00000400 - 0x000004FF - BIOS Data Area (BDA)
    // 0x00000500 - 0x000005FF - ROM BASIC (still used / reclaimed by some BIOS)

    // TODO: maybe we want to map this as MemoryType_Bootloader... There shouldn't
    // be any problem with re-using this memory once we load the kernel's IDT.
    g_memoryMap.AddBytes(MemoryType_Reserved, 0, 0, 0x600);

    // Add bootloader (ourself) to memory map
    extern const char ImageBase[];
    extern const char ImageEnd[];
    const physaddr_t start = (physaddr_t)&ImageBase;
    const physaddr_t end = (physaddr_t)&ImageEnd;
    g_memoryMap.AddBytes(MemoryType_Bootloader, MemoryFlag_ReadOnly, start, end - start);

    // Initialize the trampoline before allocating any memory
    // to ensure location 0x8000 is available to the trampoline.
    InstallBiosTrampoline();

    // Parse multiboot info
    if (magic == MULTIBOOT_BOOTLOADER_MAGIC && mbi)
    {
        m_mbi1 = (multiboot_info*)mbi;
        ParseMultibootInfo(m_mbi1);
    }
    else if (magic== MULTIBOOT2_BOOTLOADER_MAGIC && mbi)
    {
        m_mbi2 = (multiboot2_info*)mbi;
        ParseMultibootInfo(m_mbi2);
    }
    else
    {
        // TODO: we might want to display some error on the screen...
        // // Assume a standard VGA card at 0xB8000
        // g_vgaConsole.Initialize((void*)0x000B8000, 80, 25);
        // g_console = &g_vgaConsole;
        for (;;);
    }

    InitConsole();
}


void Multiboot::ParseMultibootInfo(const multiboot_info* mbi)
{
    // Add multiboot data header
    g_memoryMap.AddBytes(MemoryType_Bootloader, MemoryFlag_ReadOnly, (uintptr_t)mbi, sizeof(*mbi));

    if (mbi->flags & MULTIBOOT_MEMORY_INFO)
    {
        // Add memory map itself
        g_memoryMap.AddBytes(MemoryType_Bootloader, MemoryFlag_ReadOnly, mbi->mmap_addr, mbi->mmap_length);

        const multiboot_mmap_entry* entry = (multiboot_mmap_entry*)mbi->mmap_addr;
        const multiboot_mmap_entry* end = (multiboot_mmap_entry*)(mbi->mmap_addr + mbi->mmap_length);

        while (entry < end)
        {
            MemoryType type;
            uint32_t flags;

            switch (entry->type)
            {
            case MULTIBOOT_MEMORY_AVAILABLE:
                type = MemoryType_Available;
                flags = 0;
                break;

            case MULTIBOOT_MEMORY_ACPI_RECLAIMABLE:
                type = MemoryType_AcpiReclaimable;
                flags = 0;
                break;

            case MULTIBOOT_MEMORY_NVS:
                type = MemoryType_AcpiNvs;
                flags = 0;
                break;

            case MULTIBOOT_MEMORY_BADRAM:
                type = MemoryType_Unusable;
                flags = 0;
                break;

            case MULTIBOOT_MEMORY_RESERVED:
            default:
                type = MemoryType_Reserved;
                flags = 0;
                break;
            }

            g_memoryMap.AddBytes(type, flags, entry->addr, entry->len);

            // Go to next entry
            entry = (multiboot_mmap_entry*) ((uintptr_t)entry + entry->size + sizeof(entry->size));
        }
    }
    else if (mbi->flags & MULTIBOOT_INFO_MEMORY)
    {
        g_memoryMap.AddBytes(MemoryType_Available, 0, 0, (uint64_t)mbi->mem_lower * 1024);
        g_memoryMap.AddBytes(MemoryType_Available, 0, 1024*1024, (uint64_t)mbi->mem_upper * 1024);
    }

    if (mbi->flags & MULTIBOOT_INFO_MODS)
    {
        const multiboot_module* modules = (multiboot_module*)mbi->mods_addr;

        for (uint32_t i = 0; i != mbi->mods_count; ++i)
        {
            const multiboot_module* module = &modules[i];
            g_memoryMap.AddBytes(MemoryType_Bootloader, MemoryFlag_ReadOnly, module->mod_start, module->mod_end - module->mod_start);
        }
    }

    if (mbi->flags & MULTIBOOT_INFO_FRAMEBUFFER_INFO)
    {
        switch (mbi->framebuffer_type)
        {
        case MULTIBOOT_FRAMEBUFFER_TYPE_RGB:
            {
                const auto redMask = ((1 << mbi->framebuffer_red_mask_size) - 1) << mbi->framebuffer_red_field_position;
                const auto greenMask = ((1 << mbi->framebuffer_green_mask_size) - 1) << mbi->framebuffer_green_field_position;
                const auto blueMask = ((1 << mbi->framebuffer_blue_mask_size) - 1) << mbi->framebuffer_blue_field_position;
                const auto pixelMask = mbi->framebuffer_bpp < 32 ? (1 << mbi->framebuffer_bpp) - 1 : -1;
                const auto reservedMask = pixelMask ^ redMask ^ greenMask ^ blueMask;

                m_framebuffer.width = mbi->framebuffer_width;
                m_framebuffer.height = mbi->framebuffer_height;
                m_framebuffer.pitch = mbi->framebuffer_pitch;
                m_framebuffer.pixels = (void*)mbi->framebuffer_addr;
                m_framebuffer.format = DeterminePixelFormat(redMask, greenMask, blueMask, reservedMask);
            }
            break;

        case MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT:
            {
                // TODO: bring back VgaConsole?
                // OK to initialize VgaConsole here since it doesn't allocate any memory
                // g_vgaConsole.Initialize((void*)mbi->framebuffer_addr, mbi->framebuffer_width, mbi->framebuffer_height);
                // g_console = &g_vgaConsole;
            }
            break;
        }
    }
}


void Multiboot::ParseMultibootInfo(const multiboot2_info* mbi)
{
     // Add multiboot data header
    g_memoryMap.AddBytes(MemoryType_Bootloader, MemoryFlag_ReadOnly, (uintptr_t)mbi, mbi->total_size);

    const multiboot2_tag_basic_meminfo* meminfo = NULL;
    const multiboot2_tag_mmap* mmap = NULL;

    for (multiboot2_tag* tag = (multiboot2_tag*)(mbi + 1);
         tag->type != MULTIBOOT2_TAG_TYPE_END;
         tag = (multiboot2_tag*) align_up((uintptr_t)tag + tag->size, MULTIBOOT2_TAG_ALIGN))
    {
        switch (tag->type)
        {
        case MULTIBOOT2_TAG_TYPE_BASIC_MEMINFO:
            meminfo = (multiboot2_tag_basic_meminfo*)tag;
            break;

        case MULTIBOOT2_TAG_TYPE_MMAP:
            mmap = (multiboot2_tag_mmap*)tag;
            break;

        case MULTIBOOT2_TAG_TYPE_MODULE:
            {
                const multiboot2_module* module = (multiboot2_module*)tag;
                g_memoryMap.AddBytes(MemoryType_Bootloader, MemoryFlag_ReadOnly, module->mod_start, module->mod_end - module->mod_start);
            }
        break;

        case MULTIBOOT2_TAG_TYPE_FRAMEBUFFER:
            {
                const multiboot2_tag_framebuffer* mbi = (multiboot2_tag_framebuffer*)tag;

                switch (mbi->common.framebuffer_type)
                {
                case MULTIBOOT2_FRAMEBUFFER_TYPE_RGB:
                    {
                        const auto redMask = ((1 << mbi->framebuffer_red_mask_size) - 1) << mbi->framebuffer_red_field_position;
                        const auto greenMask = ((1 << mbi->framebuffer_green_mask_size) - 1) << mbi->framebuffer_green_field_position;
                        const auto blueMask = ((1 << mbi->framebuffer_blue_mask_size) - 1) << mbi->framebuffer_blue_field_position;
                        const auto pixelMask = mbi->common.framebuffer_bpp < 32 ? (1 << mbi->common.framebuffer_bpp) - 1 : -1;
                        const auto reservedMask = pixelMask ^ redMask ^ greenMask ^ blueMask;

                        m_framebuffer.width = mbi->common.framebuffer_width;
                        m_framebuffer.height = mbi->common.framebuffer_height;
                        m_framebuffer.pitch = mbi->common.framebuffer_pitch;
                        m_framebuffer.pixels = (void*)mbi->common.framebuffer_addr;
                        m_framebuffer.format = DeterminePixelFormat(redMask, greenMask, blueMask, reservedMask);
                    }
                    break;

                case MULTIBOOT2_FRAMEBUFFER_TYPE_EGA_TEXT:
                    {
                        // TODO: bring back VgaConsole?
                        // OK to initialize VgaConsole here since it doesn't allocate any memory
                        // g_vgaConsole.Initialize((void*)mbi->common.framebuffer_addr, mbi->common.framebuffer_width, mbi->common.framebuffer_height);
                        // g_console = &g_vgaConsole;
                    }
                    break;
                }
            }
            break;

            case MULTIBOOT2_TAG_TYPE_ACPI_OLD:
                {
                    const auto acpi = (multiboot2_tag_old_acpi*)tag;
                    if (!m_acpiRsdp) m_acpiRsdp = (Acpi::Rsdp*)acpi->rsdp;// Only set if we haven't found ACPI 2.0 yet
                }
                break;

            case MULTIBOOT2_TAG_TYPE_ACPI_NEW:
                {
                    const auto acpi = (multiboot2_tag_new_acpi*)tag;
                    m_acpiRsdp = (Acpi::Rsdp*)acpi->rsdp; // Overwrite ACPI 1.0 if it was found
                }
                break;
        }
    }

    if (mmap)
    {
        // Add memory map itself
        g_memoryMap.AddBytes(MemoryType_Bootloader, MemoryFlag_ReadOnly, (uintptr_t)mmap->entries, mmap->size);

        const multiboot2_mmap_entry* entry = mmap->entries;
        const multiboot2_mmap_entry* end = (multiboot2_mmap_entry*) ((uintptr_t)mmap + mmap->size);

        while (entry < end)
        {
            MemoryType type;
            uint32_t flags;

            switch (entry->type)
            {
            case MULTIBOOT2_MEMORY_AVAILABLE:
                type = MemoryType_Available;
                flags = 0;
                break;

            case MULTIBOOT2_MEMORY_ACPI_RECLAIMABLE:
                type = MemoryType_AcpiReclaimable;
                flags = 0;
                break;

            case MULTIBOOT2_MEMORY_NVS:
                type = MemoryType_AcpiNvs;
                flags = 0;
                break;

            case MULTIBOOT2_MEMORY_BADRAM:
                type = MemoryType_Unusable;
                flags = 0;
                break;

            case MULTIBOOT2_MEMORY_RESERVED:
            default:
                type = MemoryType_Reserved;
                flags = 0;
                break;
            }

            g_memoryMap.AddBytes(type, flags, entry->addr, entry->len);

            // Go to next entry
            entry = (multiboot2_mmap_entry*) ((uintptr_t)entry + mmap->entry_size);
        }
    }
    else if (meminfo)
    {
        g_memoryMap.AddBytes(MemoryType_Available, 0, 0, (uint64_t)meminfo->mem_lower * 1024);
        g_memoryMap.AddBytes(MemoryType_Available, 0, 1024*1024, (uint64_t)meminfo->mem_upper * 1024);
    }
}


void Multiboot::InitConsole()
{
    if (m_framebuffer.format == PIXFMT_UNKNOWN)
    {
        // TODO: VGA console?
        return;
    }

    m_display.Initialize(m_framebuffer);
    m_console.Initialize(&m_framebuffer);

    g_console = &m_console;;
}


void* Multiboot::AllocatePages(int pageCount, physaddr_t maxAddress)
{
    const physaddr_t memory = g_memoryMap.AllocatePages(MemoryType_Bootloader, pageCount, maxAddress);
    return memory == MEMORY_ALLOC_FAILED ? nullptr : (void*)memory;
}


void Multiboot::Exit(MemoryMap& memoryMap)
{
    assert(&memoryMap == &g_memoryMap);
}


static const Acpi::Rsdp* ScanMemoryForRsdp(const char* start, const char* end)
{
    for (auto p = start; p + sizeof(Acpi::Rsdp) <= end; p += 16)
    {
        if (0 == memcmp(p, "RSD PTR ", 8))
        {
            // Verify the checksum
            int checksum = 0;
            for (auto m = p; m < p + sizeof(Acpi::Rsdp); ++m)
            {
                checksum += *(unsigned char*)m;
            }

            if (0 == (checksum & 0xFF))
            {
                return (Acpi::Rsdp*)p;
            }
        }
    }

    return nullptr;
}


const Acpi::Rsdp* Multiboot::FindAcpiRsdp() const
{
    if (!m_acpiRsdp)
    {
        // Look in main BIOS area
        m_acpiRsdp = ScanMemoryForRsdp((char*)0x000e0000, (char*)0x00100000);
    }

    if (!m_acpiRsdp)
    {
        // Look in Extended BIOS Data Area
        auto ebda = (const char*)(uintptr_t)((*(uint16_t*)0x40E) << 4);
        m_acpiRsdp = ScanMemoryForRsdp(ebda, ebda + 1024);
    }

    return m_acpiRsdp;
}


int Multiboot::GetChar()
{
    // http://www.ctyme.com/intr/rb-1754.htm
    BiosRegisters regs;
    regs.ax = 0;

    CallBios(0x16, &regs, &regs);

    return regs.al;
}


int Multiboot::GetDisplayCount() const
{
    return 1;
}


IDisplay* Multiboot::GetDisplay(int index) const
{
    return index == 0 ? const_cast<VbeDisplay*>(&m_display) : nullptr;
}


bool Multiboot::LoadModule(const char* name, Module& info) const
{
    if (m_mbi1)
    {
        auto mbi = m_mbi1;

        if (mbi->flags & MULTIBOOT_INFO_MODS)
        {
            const multiboot_module* modules = (multiboot_module*)mbi->mods_addr;

            for (uint32_t i = 0; i != mbi->mods_count; ++i)
            {
                const multiboot_module* module = &modules[i];

                if (strcmp(module->string, name)==0)
                {
                    info.address = module->mod_start;
                    info.size = module->mod_end - module->mod_start;
                    return true;
                }
            }
        }
    }
    else if (m_mbi2)
    {
        for (multiboot2_tag* tag = (multiboot2_tag*)(m_mbi2 + 1);
             tag->type != MULTIBOOT2_TAG_TYPE_END;
             tag = (multiboot2_tag*) align_up((uintptr_t)tag + tag->size, MULTIBOOT2_TAG_ALIGN))
        {
            if (tag->type == MULTIBOOT2_TAG_TYPE_MODULE)
            {
                const multiboot2_module* module = (multiboot2_module*)tag;

                if (strcmp(module->string, name)==0)
                {
                    info.address = module->mod_start;
                    info.size = module->mod_end - module->mod_start;
                    return true;
                }
            }
        }
    }

    return false;
}


void Multiboot::Print(const char* string)
{
    g_console->Print(string);
}


void Multiboot::Reboot()
{
    // TODO: implement proper reset protocol (see https://forum.osdev.org/viewtopic.php?t=24623)
    // The correct* way to reset a BIOS-based x86 PC is as follows:
    //     1. Halt all cores except this one
    //     2. Write the ACPI reset vector (if present)
    //     3. If ACPI says there is an i8042, perform a keyboard-controller reset, otherwise, use a ~10µs delay
    //     4. Repeat step 2
    //     5. Repeat step 3
    //     6. Tell your user to restart their machine (as an "in case all else fails" method)
    //     7. (EFI only) Invoke the EFI shutdown system API and hope it works. It does most of the time.
    //     8. Load a zero-length IDT and cause an exception, thereby invoking a triple fault, and hope this restarts the machine (It does sometimes, but we are here because we have given up all hope of resetting the machine properly anyway)

    // For now, cause a triple fault
    asm volatile ("int $3");

    // Play safe, don't assume the above will actually work
    for (;;);
}


extern "C" void multiboot_main(unsigned int magic, const void* mbi)
{
    Multiboot multiboot(magic, mbi);

    g_bootServices->Print("Rainbow BIOS Bootloader (" STRINGIZE(KERNEL_ARCH) ")\n\n");

    Boot(&multiboot);
}
