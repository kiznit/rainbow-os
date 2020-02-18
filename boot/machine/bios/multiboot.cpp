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

#include <multiboot/multiboot.h>
#include <multiboot/multiboot2.h>

#include "bios.hpp"
#include "boot.hpp"
#include "memory.hpp"
#include "graphics/graphicsconsole.hpp"
#include "graphics/surface.hpp"

extern MemoryMap g_memoryMap;

static Surface g_frameBuffer;
static GraphicsConsole g_graphicsConsole;
static void* g_kernelAddress;
static size_t g_kernelSize;

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



void* AllocatePages(size_t pageCount, physaddr_t maxAddress)
{
    const physaddr_t memory = g_memoryMap.AllocatePages(MemoryType_Bootloader, pageCount, maxAddress);
    return memory == MEMORY_ALLOC_FAILED ? nullptr : (void*)memory;
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

            if (strcmp(module->string, "kernel")==0)
            {
                g_kernelAddress = (void*)module->mod_start;
                g_kernelSize = module->mod_end - module->mod_start;
            }
            else if (strcmp(module->string, "initrd")==0)
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
                // g_vgaConsole.Initialize((void*)mbi->framebuffer_addr, mbi->framebuffer_width, mbi->framebuffer_height);
                // g_console = &g_vgaConsole;
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

                if (strcmp(module->string, "kernel")==0)
                {
                    g_kernelAddress = (void*)module->mod_start;
                    g_kernelSize = module->mod_end - module->mod_start;
                }
                else if (strcmp(module->string, "initrd")==0)
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
                        // g_vgaConsole.Initialize((void*)mbi->common.framebuffer_addr, mbi->common.framebuffer_width, mbi->common.framebuffer_height);
                        // g_console = &g_vgaConsole;
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
                flags = 0;                break;

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


extern "C" void multiboot_main(unsigned int magic, void* mbi)
{
    // 0x00000000 - 0x000003FF - Interrupt Vector Table
    // 0x00000400 - 0x000004FF - BIOS Data Area (BDA)
    // 0x00000500 - 0x000005FF - ROM BASIC (still used / reclaimed by some BIOS)
    g_memoryMap.AddBytes(MemoryType_Bootloader, 0, 0, 0x600);

    // Process multiboot info
    bool gotMultibootInfo = false;

    // Add bootloader (ourself) to memory map
    extern const char ImageBase[];
    extern const char ImageEnd[];
    const physaddr_t start = (physaddr_t)&ImageBase;
    const physaddr_t end = (physaddr_t)&ImageEnd;
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

    if (gotMultibootInfo)
    {
        // Initialize the trampoline before allocating any memory
        // to ensure location 0x8000 is available.
        InstallBiosTrampoline();

        // Initialize a GraphicsConsole
        if (g_frameBuffer.format != PIXFMT_UNKNOWN)
        {
            g_graphicsConsole.Initialize(&g_frameBuffer);
            g_console = &g_graphicsConsole;

            Framebuffer* fb = &g_bootInfo.framebuffers[g_bootInfo.framebufferCount];

            fb->width = g_frameBuffer.width;
            fb->height = g_frameBuffer.height;
            fb->pitch = g_frameBuffer.pitch;
            fb->format = g_frameBuffer.format;
            fb->pixels = (uintptr_t)g_frameBuffer.pixels;

            g_bootInfo.framebufferCount += 1;

        }
        else if (!g_console)
        {
            // // Assume a standard VGA card at 0xB8000
            // g_vgaConsole.Initialize((void*)0x000B8000, 80, 25);
            // g_console = &g_vgaConsole;
        }
    }

    g_console->Rainbow();

    Log(" BIOS Bootloader (" STRINGIZE(KERNEL_ARCH) ")\n\n");

    Log("Kernel loaded at: %p, size: %x\n", g_kernelAddress, g_kernelSize);
    Log("initrd loaded at: %p, size: %x\n", (void*)g_bootInfo.initrdAddress, (size_t)g_bootInfo.initrdSize);

    if (gotMultibootInfo)
    {
        Boot(g_kernelAddress, g_kernelSize);
    }
    else
    {
        Fatal(" No multiboot information!\n");
    }
}
