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
#include "vgaconsole.hpp"


VgaConsole g_console;
static BootInfo g_bootInfo;
static MemoryMap g_memoryMap;


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

    // Load code segment
    asm volatile (
        "push %0\n"
        "push $1f\n"
        "retf\n"
        "1:\n"
        : : "i"(0x08) : "memory"
    );

    // Load data segments
    asm volatile (
        "movl %0, %%ds\n"
        "movl %0, %%es\n"
        "movl %0, %%fs\n"
        "movl %0, %%gs\n"
        "movl %0, %%ss\n"
        : : "r" (0x10) : "memory"
    );
}



static void ProcessMultibootInfo(multiboot_info const * const mbi)
{
    if (mbi->flags & MULTIBOOT_MEMORY_INFO)
    {
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
        // case MULTIBOOT_FRAMEBUFFER_TYPE_RGB:
        //     {
        //         FrameBufferInfo* fb = &g_frameBuffer;

        //         fb->type = FrameBufferType_RGB;
        //         fb->address = mbi->framebuffer_addr;
        //         fb->width = mbi->framebuffer_width;
        //         fb->height = mbi->framebuffer_height;
        //         fb->pitch = mbi->framebuffer_pitch;
        //         fb->bpp = mbi->framebuffer_bpp;

        //         fb->redShift = mbi->framebuffer_red_field_position;
        //         fb->redBits = mbi->framebuffer_red_mask_size;
        //         fb->greenShift = mbi->framebuffer_green_field_position;
        //         fb->greenBits = mbi->framebuffer_green_mask_size;
        //         fb->blueShift = mbi->framebuffer_blue_field_position;
        //         fb->blueBits = mbi->framebuffer_blue_mask_size;

        //         g_bootInfo.frameBufferCount = 1;
        //         g_bootInfo.framebuffers = (uintptr_t)fb;
        //     }
        //     break;

        case MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT:
            {
                g_console.Initialize((void*)mbi->framebuffer_addr, mbi->framebuffer_width, mbi->framebuffer_height);

                // FrameBufferInfo* fb = &g_frameBuffer;

                // fb->type = FrameBufferType_VGAText;
                // fb->address = mbi->framebuffer_addr;
                // fb->width = mbi->framebuffer_width;
                // fb->height = mbi->framebuffer_height;
                // fb->pitch = mbi->framebuffer_pitch;
                // fb->bpp = mbi->framebuffer_bpp;

                // g_bootInfo.frameBufferCount = 1;
                // g_bootInfo.framebuffers = (uintptr_t)fb;
            }
            break;
        }
    }
}



static void ProcessMultibootInfo(multiboot2_info const * const mbi)
{
    const multiboot2_tag_basic_meminfo* meminfo = NULL;
    const multiboot2_tag_mmap* mmap = NULL;

    for (multiboot2_tag* tag = (multiboot2_tag*)(mbi + 1);
         tag->type != MULTIBOOT2_TAG_TYPE_END;
         tag = (multiboot2_tag*) (((uintptr_t)tag + tag->size + MULTIBOOT2_TAG_ALIGN - 1) & ~(MULTIBOOT2_TAG_ALIGN - 1)))
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
                // case MULTIBOOT2_FRAMEBUFFER_TYPE_RGB:
                //     {
                //         FrameBufferInfo* fb = &g_frameBuffer;

                //         fb->type = FrameBufferType_RGB;
                //         fb->address = mbi->common.framebuffer_addr;
                //         fb->width = mbi->common.framebuffer_width;
                //         fb->height = mbi->common.framebuffer_height;
                //         fb->pitch = mbi->common.framebuffer_pitch;
                //         fb->bpp = mbi->common.framebuffer_bpp;

                //         fb->redShift = mbi->framebuffer_red_field_position;
                //         fb->redBits = mbi->framebuffer_red_mask_size;
                //         fb->greenShift = mbi->framebuffer_green_field_position;
                //         fb->greenBits = mbi->framebuffer_green_mask_size;
                //         fb->blueShift = mbi->framebuffer_blue_field_position;
                //         fb->blueBits = mbi->framebuffer_blue_mask_size;

                //         g_bootInfo.frameBufferCount = 1;
                //         g_bootInfo.framebuffers = (uintptr_t)fb;
                //     }
                //     break;

                case MULTIBOOT2_FRAMEBUFFER_TYPE_EGA_TEXT:
                    {
                        g_console.Initialize((void*)mbi->common.framebuffer_addr, mbi->common.framebuffer_width, mbi->common.framebuffer_height);

                        // FrameBufferInfo* fb = &g_frameBuffer;

                        // fb->type = FrameBufferType_VGAText;
                        // fb->address = mbi->common.framebuffer_addr;
                        // fb->width = mbi->common.framebuffer_width;
                        // fb->height = mbi->common.framebuffer_height;
                        // fb->pitch = mbi->common.framebuffer_pitch;
                        // fb->bpp = mbi->common.framebuffer_bpp;

                        // g_bootInfo.frameBufferCount = 1;
                        // g_bootInfo.framebuffers = (uintptr_t)fb;
                    }
                    break;
                }
            }
            break;
        }
    }

    if (mmap)
    {
        const multiboot2_mmap_entry* entry = mmap->entries;
        const multiboot2_mmap_entry* end = (multiboot2_mmap_entry*) (((uintptr_t)mmap + mmap->size + MULTIBOOT2_TAG_ALIGN - 1) & ~(MULTIBOOT2_TAG_ALIGN - 1));

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


extern "C" void multiboot_main(unsigned int magic, void* mbi)
{
    // Initialize the GDT so that we have valid 16-bit segments to work with BIOS calls
    InitGDT();

    // Process multiboot info
    bool gotMultibootInfo = false;

    // Assume a standard VGA card at 0xB8000 =)
    g_console.Initialize((void*)0x000B8000, 80, 25);

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

    g_console.Rainbow();

    printf(" BIOS Loader\n\n");

    if (gotMultibootInfo)
    {
        // Block out BIOS areas from memory map
        g_memoryMap.AddBytes(MemoryType_Bootloader, 0, 0, 0x500);   // Interrupt Vector Table (0x400) + BIOS Data Area (0x100)

        // ROM / Video / BIOS reserved memory area (0xA0000 - 0xFFFFF)
        g_memoryMap.AddBytes(MemoryType_Reserved, 0, 0xA0000, 0x60000);

        // Install trampoline for BIOS calls
        extern const char BiosTrampolineStart[];
        extern const char BiosTrampolineEnd[];
        extern const char BiosStackTop[];

        const auto trampolineSize = BiosTrampolineEnd - BiosTrampolineStart;
        g_memoryMap.AddBytes(MemoryType_Bootloader, 0, 0x8000, BiosStackTop - BiosTrampolineStart);
        memcpy((void*) 0x8000, BiosTrampolineStart, trampolineSize);

        // Boot!
        Boot(&g_bootInfo, &g_memoryMap);
    }
    else
    {
        printf("FATAL: No multiboot information!\n");
    }
}
