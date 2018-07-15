/*
    Copyright (c) 2017, Thierry Tremblay
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
#include <string.h>
#include <multiboot.h>
#include <multiboot2.h>

#include "bios.hpp"
#include "boot.hpp"
#include "memory.hpp"
#include "vbedisplay.hpp"
#include "vgaconsole.hpp"
#include "graphics/graphicsconsole.hpp"
#include "graphics/surface.hpp"



static Surface g_frameBuffer;
static VbeDisplay g_display;
static VgaConsole g_vgaConsole;
static GraphicsConsole g_graphicsConsole;
IConsole* g_console = nullptr;


/*
    Multiboot structures
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


struct gdt_descriptor
{
    uint16_t limit;
    uint16_t base;
    uint16_t flags1;
    uint16_t flags2;
};


struct gdt_ptr
{
    uint16_t size;
    void* address;
} __attribute__((packed));


static gdt_descriptor GDT[] __attribute__((aligned(16))) =
{
    // 0x00 - Null descriptor
    { 0, 0, 0, 0 },

    // 0x08 - 32-bit code descriptor
    {
        0xFFFF,     // Limit = 0x100000 * 4 KB = 4 GB
        0x0000,     // Base = 0
        0x9A00,     // P + DPL 0 + S + Code + Execute + Read
        0x00CF,     // G + D (32 bits)
    },

    // 0x10 - 32-bit data descriptor
    {
        0xFFFF,     // Limit = 0x100000 * 4 KB = 4 GB
        0x0000,     // Base = 0
        0x9200,     // P + DPL 0 + S + Data + Read + Write
        0x00CF,     // G + B (32 bits)
    },

    // 0x18 - 16-bit code descriptor
    {
        0xFFFF,     // Limit = 0x100000 = 1 MB
        0x0000,     // Base = 0
        0x9A00,     // P + DPL 0 + S + Code + Execute + Read
        0x000F,     // Limit (top 4 bits)
    },
/*
    // 0x20 - 16-bit data descriptor
    {
        0xFFFF,     // Limit = 0x100000 = 1 MB
        0x0000,     // Base = 0
        0x9200,     // P + DPL 0 + S + Data + Read + Write
        0x000F,     // Limit (top 4 bits)
    }
*/
};


static gdt_ptr GDT_PTR =
{
    sizeof(GDT)-1,
    GDT
};


static void InitGDT()
{
    // Load GDT
    asm volatile ("lgdt %0" : : "m" (GDT_PTR) );

    // Load segment descriptors
    asm volatile (
        "ljmpl %0, $1f\n"
        "1:\n"
        "movl %1, %%ds\n"
        "movl %1, %%es\n"
        "movl %1, %%fs\n"
        "movl %1, %%gs\n"
        "movl %1, %%ss\n"
        : : "i" (0x08), "r" (0x10) : "memory"
    );
}



void* AllocatePages(size_t pageCount, uintptr_t maxAddress)
{
    const physaddr_t memory = g_memoryMap.AllocatePages(MemoryType_Bootloader, pageCount, maxAddress);
    if (memory == MEMORY_ALLOC_FAILED)
    {
        return nullptr;
    }

    return (void*)memory;
}



bool FreePages(void* memory, size_t pageCount)
{
    // TODO: do we want to implement this?
    (void)memory;
    (void)pageCount;

    return true;
}



static void ProcessMultibootInfo(multiboot_info const * const mbi)
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

            if (strcmp(module->string, "initrd")==0)
            {
                g_bootInfo.initrdAddress = module->mod_start;
                g_bootInfo.initrdSize = module->mod_end - module->mod_start;
            }

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

                g_frameBuffer.width = mbi->framebuffer_width;
                g_frameBuffer.height = mbi->framebuffer_height;
                g_frameBuffer.pitch = mbi->framebuffer_pitch;
                g_frameBuffer.pixels = (void*)mbi->framebuffer_addr;
                g_frameBuffer.format = DeterminePixelFormat(redMask, greenMask, blueMask, reservedMask);
            }
            break;

        case MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT:
            {
                // OK to initialize VgaConsole here since it doesn't allocate any memory
                g_vgaConsole.Initialize((void*)mbi->framebuffer_addr, mbi->framebuffer_width, mbi->framebuffer_height);
                g_console = &g_vgaConsole;
            }
            break;
        }
    }
}



static void ProcessMultibootInfo(multiboot2_info const * const mbi)
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

                if (strcmp(module->string, "initrd")==0)
                {
                    g_bootInfo.initrdAddress = module->mod_start;
                    g_bootInfo.initrdSize = module->mod_end - module->mod_start;
                }

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

                        g_frameBuffer.width = mbi->common.framebuffer_width;
                        g_frameBuffer.height = mbi->common.framebuffer_height;
                        g_frameBuffer.pitch = mbi->common.framebuffer_pitch;
                        g_frameBuffer.pixels = (void*)mbi->common.framebuffer_addr;
                        g_frameBuffer.format = DeterminePixelFormat(redMask, greenMask, blueMask, reservedMask);
                    }
                    break;

                case MULTIBOOT2_FRAMEBUFFER_TYPE_EGA_TEXT:
                    {
                        // OK to initialize VgaConsole here since it doesn't allocate any memory
                        g_vgaConsole.Initialize((void*)mbi->common.framebuffer_addr, mbi->common.framebuffer_width, mbi->common.framebuffer_height);
                        g_console = &g_vgaConsole;
                    }
                    break;
                }
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



void InstallBiosTrampoline()
{
    extern const char BiosTrampolineStart[];
    extern const char BiosTrampolineEnd[];
    extern const char BiosStackTop[];

    const uintptr_t address = 0x8000;

    const auto trampolineSize = BiosTrampolineEnd - BiosTrampolineStart;
    g_memoryMap.AddBytes(MemoryType_Bootloader, 0, address, BiosStackTop - BiosTrampolineStart);
    memcpy((void*)address, BiosTrampolineStart, trampolineSize);
}



extern "C" void multiboot_main(unsigned int magic, void* mbi)
{
    CallGlobalConstructors();

    // Block out BIOS areas from memory map
    g_memoryMap.AddBytes(MemoryType_Bootloader, 0, 0, 0x500);   // Interrupt Vector Table (0x400) + BIOS Data Area (0x100)

    // ROM / Video / BIOS reserved memory area (0xA0000 - 0xFFFFF)
    g_memoryMap.AddBytes(MemoryType_Reserved, 0, 0xA0000, 0x60000);

    // Initialize the GDT so that we have valid 16-bit segments to work with BIOS calls
    InitGDT();

    // Process multiboot info
    bool gotMultibootInfo = false;

    // Make sure we don't think there is a graphics framebuffer when there isn't one
    g_frameBuffer.format = PIXFMT_UNKNOWN;

    // Add bootloader (ourself) to memory map
    extern const char bootloader_image_start[];
    extern const char bootloader_image_end[];
    const physaddr_t start = (physaddr_t)&bootloader_image_start;
    const physaddr_t end = (physaddr_t)&bootloader_image_end;
    g_memoryMap.AddBytes(MemoryType_Bootloader, MemoryFlag_ReadOnly, start, end - start);


    if (magic == MULTIBOOT_BOOTLOADER_MAGIC && mbi)
    {
        ProcessMultibootInfo(static_cast<multiboot_info*>(mbi));
        gotMultibootInfo = true;
    }
    else if (magic== MULTIBOOT2_BOOTLOADER_MAGIC && mbi)
    {
        ProcessMultibootInfo(static_cast<multiboot2_info*>(mbi));
        gotMultibootInfo = true;
    }

    // Now that the memory allocator is initialized, we can create GraphicsConsole
    if (gotMultibootInfo)
    {
        if (g_frameBuffer.format != PIXFMT_UNKNOWN)
        {
            g_graphicsConsole.Initialize(&g_frameBuffer);
            g_console = &g_graphicsConsole;
        }
        else if (!g_console)
        {
            // Assume a standard VGA card at 0xB8000 =)
            g_vgaConsole.Initialize((void*)0x000B8000, 80, 25);
            g_console = &g_vgaConsole;
        }
    }

    g_console->Rainbow();

    printf(" BIOS Loader\n\n");

    if (gotMultibootInfo)
    {
        InstallBiosTrampoline();

        g_display.Initialize();

        for (int i = 0; i != g_display.GetModeCount(); ++i)
        {
            DisplayMode mode;
            g_display.GetMode(i, &mode);

            if (mode.format == PIXFMT_X8R8G8B8)
            {
                //printf("Mode %d: %d x %d - %d\n", i, mode.width, mode.height, mode.format);
            }
        }

        // Boot!
        Boot();
    }
    else
    {
        printf("FATAL: No multiboot information!\n");
    }

    CallGlobalDestructors();
}
