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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <multiboot.h>
#include <multiboot2.h>

#include "boot.hpp"
#include "elf.hpp"
#include "memory.hpp"
#include "vgaconsole.hpp"


struct Module
{
    physaddr_t  start;
    physaddr_t  end;
    const char* name;
};


// Globals
static VgaConsole g_vgaConsole;
VgaConsole* g_console = NULL;

static BootInfo g_bootInfo;
MemoryMap g_memoryMap;

static Module g_modules[100];
static int g_moduleCount = 0;



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



/*
    Boot - Load kernel and execute it
*/

static void Boot()
{
    g_bootInfo.version = RAINBOW_BOOT_VERSION;
    g_bootInfo.firmware = Firmware_BIOS;

    g_memoryMap.Print();

    // const ModuleInfo* kernel = g_modules.FindModule("kernel");
    // if (!kernel)
    // {
    //     printf("Module not found: kernel\n");
    //     return;
    // }

    // Elf32Loader elf((char*)kernel->start, kernel->end - kernel->start);
    // if (!elf.Valid())
    // {
    //     printf("kernel: invalid ELF file\n");
    //     return;
    // }

    // if (elf.GetType() != ET_DYN)
    // {
    //     printf("kernel: module is not a shared object file\n");
    //     return;
    // }

    // const unsigned int size = elf.GetMemorySize();
    // const unsigned int alignment = elf.GetMemoryAlignment();
    // const int pageCount = MEMORY_ROUND_PAGE_UP(size) >> MEMORY_PAGE_SHIFT;

    // void* memory = NULL;

    // if (alignment <= MEMORY_PAGE_SIZE)
    // {
    //     const physaddr_t address = g_memoryMap.AllocatePages(MemoryType_Bootloader, pageCount, RAINBOW_KERNEL_BASE_ADDRESS);
    //     if (address != (physaddr_t)-1)
    //     {
    //         memory = (void*)address;
    //     }
    // }

    // if (!memory)
    // {
    //     printf("Could not allocate memory to load kernel (size: %u, alignment: %u)\n", size, alignment);
    //     return;
    // }

    // printf("Kernel memory allocated at %p\n", memory);

    // void* entry = elf.Load(memory);
    // if (entry == NULL)
    // {
    //     printf("Error loading kernel\n");
    //     return;
    // }


    // printf("kernel_main() at %p\n", entry);

    // g_memoryMap.Sanitize();

    // // putchar('\n');
    // // g_memoryMap.Print();
    // // putchar('\n');
    // // g_modules.Print();
    // // putchar('\n');

    // // Jump to kernel
    // typedef void (*kernel_entry_t)(const BootInfo*);
    // kernel_entry_t kernel_main = (kernel_entry_t)entry;
    // kernel_main(&g_bootInfo);
}



static void ProcessMultibootInfo(multiboot_info const * const mbi)
{
    if (mbi->flags & MULTIBOOT_MEMORY_INFO)
    {
        const multiboot_mmap_entry* entry = (multiboot_mmap_entry*)mbi->mmap_addr;
        const multiboot_mmap_entry* end = (multiboot_mmap_entry*)(mbi->mmap_addr + mbi->mmap_length);

        while (entry < end)
        {
            MemoryType type = MemoryType_Reserved;

            switch (entry->type)
            {
            case MULTIBOOT_MEMORY_AVAILABLE:
                type = MemoryType_Available;
                break;

            case MULTIBOOT_MEMORY_RESERVED:
                type = MemoryType_Reserved;
                break;

            case MULTIBOOT_MEMORY_ACPI_RECLAIMABLE:
                type = MemoryType_AcpiReclaimable;
                break;

            case MULTIBOOT_MEMORY_NVS:
                type = MemoryType_AcpiNvs;
                break;

            case MULTIBOOT_MEMORY_BADRAM:
                type = MemoryType_Unusable;
                break;
            }

            g_memoryMap.AddBytes(type, entry->addr, entry->len);

            // Go to next entry
            entry = (multiboot_mmap_entry*) ((uintptr_t)entry + entry->size + sizeof(entry->size));
        }
    }
    else if (mbi->flags & MULTIBOOT_INFO_MEMORY)
    {
        g_memoryMap.AddBytes(MemoryType_Available, 0, (uint64_t)mbi->mem_lower * 1024);
        g_memoryMap.AddBytes(MemoryType_Available, 1024*1024, (uint64_t)mbi->mem_upper * 1024);
    }

    if (mbi->flags & MULTIBOOT_INFO_MODS)
    {
        const multiboot_module* modules = (multiboot_module*)mbi->mods_addr;

        for (uint32_t i = 0; i != mbi->mods_count; ++i)
        {
            const multiboot_module* module = &modules[i];

            g_memoryMap.AddBytes(MemoryType_Bootloader, module->mod_start, module->mod_end - module->mod_start);

            if (g_moduleCount != ARRAY_LENGTH(g_modules))
            {
                g_modules[g_moduleCount].start = module->mod_start;
                g_modules[g_moduleCount].end = module->mod_end;
                g_modules[g_moduleCount].name = module->string;
                ++g_moduleCount;
            }
        }
    }

    if (mbi->flags & MULTIBOOT_INFO_FRAMEBUFFER_INFO)
    {
        if (mbi->framebuffer_type == MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT)
        {
            g_vgaConsole.Initialize((void*)mbi->framebuffer_addr, mbi->framebuffer_width, mbi->framebuffer_height);
            g_console = &g_vgaConsole;
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

                g_memoryMap.AddBytes(MemoryType_Bootloader, module->mod_start, module->mod_end - module->mod_start);

                if (g_moduleCount != ARRAY_LENGTH(g_modules))
                {
                    g_modules[g_moduleCount].start = module->mod_start;
                    g_modules[g_moduleCount].end = module->mod_end;
                    g_modules[g_moduleCount].name = module->string;
                    ++g_moduleCount;
                }
            }
        break;

        case MULTIBOOT2_TAG_TYPE_FRAMEBUFFER:
            {
                const multiboot2_tag_framebuffer* fb = (multiboot2_tag_framebuffer*)tag;

                if (fb->common.framebuffer_type == MULTIBOOT2_FRAMEBUFFER_TYPE_EGA_TEXT)
                {
                    g_vgaConsole.Initialize((void*)fb->common.framebuffer_addr, fb->common.framebuffer_width, fb->common.framebuffer_height);
                    g_console = &g_vgaConsole;
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
            MemoryType type = MemoryType_Reserved;

            switch (entry->type)
            {
            case MULTIBOOT_MEMORY_AVAILABLE:
                type = MemoryType_Available;
                break;

            case MULTIBOOT_MEMORY_RESERVED:
                type = MemoryType_Reserved;
                break;

            case MULTIBOOT_MEMORY_ACPI_RECLAIMABLE:
                type = MemoryType_AcpiReclaimable;
                break;

            case MULTIBOOT_MEMORY_NVS:
                type = MemoryType_AcpiNvs;
                break;

            case MULTIBOOT_MEMORY_BADRAM:
                type = MemoryType_Unusable;
                break;
            }

            g_memoryMap.AddBytes(type, entry->addr, entry->len);

            // Go to next entry
            entry = (multiboot2_mmap_entry*) ((uintptr_t)entry + mmap->entry_size);
        }
    }
    else if (meminfo)
    {
        g_memoryMap.AddBytes(MemoryType_Available, 0, (uint64_t)meminfo->mem_lower * 1024);
        g_memoryMap.AddBytes(MemoryType_Available, 1024*1024, (uint64_t)meminfo->mem_upper * 1024);
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



static void Initialize()
{
    CallGlobalConstructors();
}



static void Shutdown()
{
    printf("\nExiting...");

    CallGlobalDestructors();
}



extern "C" void multiboot_main(unsigned int magic, void* mbi)
{
    Initialize();

    // Add bootloader (ourself) to memory map
    extern const char bootloader_image_start[];
    extern const char bootloader_image_end[];

    const physaddr_t start = (physaddr_t)&bootloader_image_start;
    const physaddr_t end = (physaddr_t)&bootloader_image_end;
    g_memoryMap.AddBytes(MemoryType_Bootloader, start, end - start);

    // Process multiboot info
    bool gotMultibootInfo = false;

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
    else
    {
        // No multiboot header, hope there is a standard VGA card at 0xB8000 =)
        g_vgaConsole.Initialize((void*)0x000B8000, 80, 25);
        g_console = &g_vgaConsole;
    }

    // Welcome message
    if (g_console)
    {
        g_console->Rainbow();
        printf(" Multiboot Bootloader\n\n");
    }

    if (gotMultibootInfo)
    {
        Boot();
    }
    else
    {
        printf("FATAL: No multiboot information!\n");
    }

    Shutdown();
}
