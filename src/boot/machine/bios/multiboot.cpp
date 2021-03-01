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
#include <cassert>
#include <cstring>
#include <multiboot/multiboot.h>
#include <multiboot/multiboot2.h>
#include "bios.hpp"
#include "memory.hpp"
#include "graphics/surface.hpp"

extern MemoryMap g_memoryMap;


/*
    Some information on BIOS memory maps (E820)

    From http://www.uruk.org/orig-grub/mem64mb.html:

    Assumptions and Limitations
        1. The BIOS will return address ranges describing base board memory and
           ISA or PCI memory that is contiguous with that baseboard memory.
        2. The BIOS WILL NOT return a range description for the memory mapping
           of PCI devices, ISA Option ROM's, and ISA plug & play cards. This is
        because the OS has mechanisms available to detect them.
        3. The BIOS will return chipset defined address holes that are not being
           used by devices as reserved.
        4. Address ranges defined for base board memory mapped I/O devices (for
           example APICs) will be returned as reserved.
        5. All occurrences of the system BIOS will be mapped as reserved. This
           includes the area below 1 MB, at 16 MB (if present) and at end of the
           address space (4 gig).
        6. Standard PC address ranges will not be reported. Example video memory
           at A0000 to BFFFF physical will not be described by this function. The
           range from E0000 to EFFFF is base board specific and will be reported
           as suits the base board.
        7. All of lower memory is reported as normal memory. It is OS's responsibility
           to handle standard RAM locations reserved for specific uses, for example:
           the interrupt vector table(0:0) and the BIOS data area(40:0).
*/


/*
    E820 can report persistent memory, but multiboot has no definitions for them.
*/

// Types 7 and 14 are defined by ACPI 6.0
#define E820_TYPE_PMEM 7
#define E820_TYPE_PRAM 14
// Type 12 was used by some OEM before ACPI 6.0
#define E820_TYPE_PRAM_LEGACY 12


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
:   m_mbi1(nullptr),
    m_mbi2(nullptr),
    m_acpiRsdp(nullptr)
{
    // We do this so that the CRT can be used right away.
    g_bootServices = this;

    // 0x00000000 - 0x000003FF - Interrupt Vector Table
    // 0x00000400 - 0x000004FF - BIOS Data Area (BDA)
    // 0x00000500 - 0x000005FF - ROM BASIC (still used / reclaimed by some BIOS)
    g_memoryMap.AddBytes(MemoryType::Bootloader, 0, 0x600);

    // Add bootloader (ourself) to memory map
    extern const char ImageBase[];
    extern const char ImageEnd[];
    const physaddr_t start = (physaddr_t)&ImageBase;
    const physaddr_t end = (physaddr_t)&ImageEnd;
    g_memoryMap.AddBytes(MemoryType::Bootloader, start, end - start);

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
    g_memoryMap.AddBytes(MemoryType::Bootloader, (uintptr_t)mbi, sizeof(*mbi));

    if (mbi->flags & MULTIBOOT_MEMORY_INFO)
    {
        // Add memory map itself
        g_memoryMap.AddBytes(MemoryType::Bootloader, mbi->mmap_addr, mbi->mmap_length);

        const multiboot_mmap_entry* entry = (multiboot_mmap_entry*)mbi->mmap_addr;
        const multiboot_mmap_entry* end = (multiboot_mmap_entry*)(mbi->mmap_addr + mbi->mmap_length);

        while (entry < end)
        {
            MemoryType type;

            switch (entry->type)
            {
            case MULTIBOOT_MEMORY_AVAILABLE:
                type = MemoryType::Available;
                break;

            case MULTIBOOT_MEMORY_ACPI_RECLAIMABLE:
                type = MemoryType::AcpiReclaimable;
                break;

            case MULTIBOOT_MEMORY_NVS:
                type = MemoryType::AcpiNvs;
                break;

            case MULTIBOOT_MEMORY_BADRAM:
                type = MemoryType::Unusable;
                break;

            case E820_TYPE_PMEM:
            case E820_TYPE_PRAM:
            case E820_TYPE_PRAM_LEGACY:
                type = MemoryType::Persistent;
                break;

            case MULTIBOOT_MEMORY_RESERVED:
            default:
                // Check for BIOS EDBA
                // TODO: we need to do better, see https://github.com/spotify/linux/blob/master/arch/x86/kernel/head.c
                //       or maybe Grub handles this for us...
                if (entry->addr + entry->len == 0xA0000)
                {
                    type = MemoryType::Bootloader;
                }
                else
                {
                    type = MemoryType::Reserved;
                }
                break;
            }

            g_memoryMap.AddBytes(type, entry->addr, entry->len);

            // Go to next entry
            entry = (multiboot_mmap_entry*) ((uintptr_t)entry + entry->size + sizeof(entry->size));
        }
    }
    else if (mbi->flags & MULTIBOOT_INFO_MEMORY)
    {
        g_memoryMap.AddBytes(MemoryType::Available, 0, (uint64_t)mbi->mem_lower * 1024);
        g_memoryMap.AddBytes(MemoryType::Available, 1024*1024, (uint64_t)mbi->mem_upper * 1024);
    }

    if (mbi->flags & MULTIBOOT_INFO_MODS)
    {
        const multiboot_module* modules = (multiboot_module*)mbi->mods_addr;

        for (uint32_t i = 0; i != mbi->mods_count; ++i)
        {
            const multiboot_module* module = &modules[i];
            g_memoryMap.AddBytes(MemoryType::Bootloader, module->mod_start, module->mod_end - module->mod_start);
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
    g_memoryMap.AddBytes(MemoryType::Bootloader, (uintptr_t)mbi, mbi->total_size);

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
                g_memoryMap.AddBytes(MemoryType::Bootloader, module->mod_start, module->mod_end - module->mod_start);
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
                    // Only set if we haven't found ACPI 2.0 yet
                    if (!m_acpiRsdp)
                    {
                        const auto acpi = (multiboot2_tag_old_acpi*)tag;
                        const auto rsdp = (Acpi::Rsdp*)acpi->rsdp;
                        if (rsdp && rsdp->VerifyChecksum())
                        {
                            m_acpiRsdp = rsdp;
                        }
                    }
                }
                break;

            case MULTIBOOT2_TAG_TYPE_ACPI_NEW:
                {
                    // Always override ACPI 1.0
                    const auto acpi = (multiboot2_tag_new_acpi*)tag;
                    const auto rsdp = (Acpi::Rsdp20*)acpi->rsdp;
                    if (rsdp && rsdp->VerifyExtendedChecksum())
                    {
                        m_acpiRsdp = rsdp;
                    }
                }
                break;
        }
    }

    if (mmap)
    {
        // Add memory map itself
        g_memoryMap.AddBytes(MemoryType::Bootloader, (uintptr_t)mmap->entries, mmap->size);

        const multiboot2_mmap_entry* entry = mmap->entries;
        const multiboot2_mmap_entry* end = (multiboot2_mmap_entry*) ((uintptr_t)mmap + mmap->size);

        while (entry < end)
        {
            MemoryType type;

            switch (entry->type)
            {
            case MULTIBOOT2_MEMORY_AVAILABLE:
                type = MemoryType::Available;
                break;

            case MULTIBOOT2_MEMORY_ACPI_RECLAIMABLE:
                type = MemoryType::AcpiReclaimable;
                break;

            case MULTIBOOT2_MEMORY_NVS:
                type = MemoryType::AcpiNvs;
                break;

            case MULTIBOOT2_MEMORY_BADRAM:
                type = MemoryType::Unusable;
                break;

            case E820_TYPE_PMEM:
            case E820_TYPE_PRAM:
            case E820_TYPE_PRAM_LEGACY:
                type = MemoryType::Persistent;
                break;

            case MULTIBOOT2_MEMORY_RESERVED:
            default:
                // Check for BIOS EDBA
                // TODO: we need to do better, see https://github.com/spotify/linux/blob/master/arch/x86/kernel/head.c
                //       or maybe Grub handles this for us...
                if (entry->addr + entry->len == 0xA0000)
                {
                    type = MemoryType::Bootloader;
                }
                else
                {
                    type = MemoryType::Reserved;
                }
                break;
            }

            g_memoryMap.AddBytes(type, entry->addr, entry->len);

            // Go to next entry
            entry = (multiboot2_mmap_entry*) ((uintptr_t)entry + mmap->entry_size);
        }
    }
    else if (meminfo)
    {
        g_memoryMap.AddBytes(MemoryType::Available, 0, (uint64_t)meminfo->mem_lower * 1024);
        g_memoryMap.AddBytes(MemoryType::Available, 1024*1024, (uint64_t)meminfo->mem_upper * 1024);
    }
}


void Multiboot::InitConsole()
{
    if (m_framebuffer.format == PixelFormat::Unknown)
    {
        // TODO: VGA console?
        return;
    }

    m_display.Initialize(m_framebuffer);
    m_console.Initialize(&m_framebuffer, &m_framebuffer);

    g_console = &m_console;;
}


physaddr_t Multiboot::AllocatePages(int pageCount, physaddr_t maxAddress)
{
    return g_memoryMap.AllocatePages(MemoryType::Bootloader, pageCount, maxAddress);
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
            const auto rsdp = (Acpi::Rsdp20*)p;

            // Verify the checksum
            if (rsdp->VerifyChecksum())
            {
                // ACPI 1.0
                if (rsdp->revision < 2)
                {
                    return rsdp;
                }

                // ACPI 2.0
                if (rsdp->VerifyExtendedChecksum())
                {
                    return rsdp;
                }
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


void Multiboot::Print(const char* string, size_t length)
{
    g_console->Print(string, length);
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

    Log("Rainbow BIOS Bootloader (" STRINGIZE(KERNEL_ARCH) ")\n\n");

    Boot(&multiboot);
}
