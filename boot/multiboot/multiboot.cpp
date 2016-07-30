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

#include <vgaconsole.hpp>

#include "../common/boot.hpp"
#include "../common/elf.hpp"
#include "../common/memory.hpp"


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
static FrameBufferInfo g_frameBuffer;
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



    StartKernel32(&g_bootInfo, pdpt, entry);

    printf("kernel_main() returned!\n");

    abort();
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


    StartKernel64(&g_bootInfo, PML4, entry);

    printf("kernel_main() returned!\n");

    abort();
}



/*
    Boot - Load kernel and execute it
*/

static void Boot()
{
    // Find the kernel module
    const Module* kernel = NULL;

    for (int i = 0; i != g_moduleCount; ++i)
    {
        if (strcmp(g_modules[i].name, "kernel") == 0)
        {
            kernel = &g_modules[i];
            break;
        }
    }

    if (!kernel)
    {
        printf("Could not find kernel in multiboot modules\n");
        return;
    }

    ElfLoader elf((char*)kernel->start, kernel->end - kernel->start);
    if (!elf.Valid())
    {
        printf("Unsupported: \"kernel\" is not a valid elf file\n");
        return;
    }

    if (elf.GetType() != ET_EXEC)
    {
        printf("Unsupported: \"kernel\" is not an executable\n");
        return;
    }

    const unsigned int size = elf.GetMemorySize();
    const unsigned int alignment = elf.GetMemoryAlignment();

    void* memory = NULL;

    if (alignment <= MEMORY_PAGE_SIZE)
    {
        const physaddr_t address = g_memoryMap.AllocateBytes(MemoryType_Kernel, size);
        if (address != (physaddr_t)-1)
        {
            memory = (void*)address;
        }
    }

    if (!memory)
    {
        printf("Could not allocate memory to load kernel (size: %u, alignment: %u)\n", size, alignment);
        return;
    }

    printf("Kernel memory allocated at %p - %p\n", memory, (char*)memory + size);

    physaddr_t entry = elf.Load(memory);
    if (entry == 0)
    {
        printf("Error loading kernel\n");
        return;
    }

    g_memoryMap.Sanitize();
    g_memoryMap.Print();

    g_bootInfo.memoryDescriptorCount = g_memoryMap.size();
    g_bootInfo.memoryDescriptors = (uintptr_t)g_memoryMap.begin();

    // Execute kernel
    if (elf.Is32Bits())
    {
        Boot32(elf.GetStartAddress(), entry, memory, size);
    }
    else
    {
        Boot64(elf.GetStartAddress(), entry, memory, size);
    }
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

            g_memoryMap.AddBytes(MemoryType_Bootloader, MemoryFlag_ReadOnly, module->mod_start, module->mod_end - module->mod_start);

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
        switch (mbi->framebuffer_type)
        {
        case MULTIBOOT_FRAMEBUFFER_TYPE_RGB:
            {
                FrameBufferInfo* fb = &g_frameBuffer;

                fb->type = FrameBufferType_RGB;
                fb->address = mbi->framebuffer_addr;
                fb->width = mbi->framebuffer_width;
                fb->height = mbi->framebuffer_height;
                fb->pitch = mbi->framebuffer_pitch;
                fb->bpp = mbi->framebuffer_bpp;

                fb->redShift = mbi->framebuffer_red_field_position;
                fb->redBits = mbi->framebuffer_red_mask_size;
                fb->greenShift = mbi->framebuffer_green_field_position;
                fb->greenBits = mbi->framebuffer_green_mask_size;
                fb->blueShift = mbi->framebuffer_blue_field_position;
                fb->blueBits = mbi->framebuffer_blue_mask_size;

                g_bootInfo.frameBufferCount = 1;
                g_bootInfo.framebuffers = (uintptr_t)fb;
            }
            break;

        case MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT:
            {
                g_vgaConsole.Initialize((void*)mbi->framebuffer_addr, mbi->framebuffer_width, mbi->framebuffer_height);
                g_console = &g_vgaConsole;

                FrameBufferInfo* fb = &g_frameBuffer;

                fb->type = FrameBufferType_VGAText;
                fb->address = mbi->framebuffer_addr;
                fb->width = mbi->framebuffer_width;
                fb->height = mbi->framebuffer_height;
                fb->pitch = mbi->framebuffer_pitch;
                fb->bpp = mbi->framebuffer_bpp;

                g_bootInfo.frameBufferCount = 1;
                g_bootInfo.framebuffers = (uintptr_t)fb;
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

                g_memoryMap.AddBytes(MemoryType_Bootloader, MemoryFlag_ReadOnly, module->mod_start, module->mod_end - module->mod_start);

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
                const multiboot2_tag_framebuffer* mbi = (multiboot2_tag_framebuffer*)tag;

                switch (mbi->common.framebuffer_type)
                {
                case MULTIBOOT2_FRAMEBUFFER_TYPE_RGB:
                    {
                        FrameBufferInfo* fb = &g_frameBuffer;

                        fb->type = FrameBufferType_RGB;
                        fb->address = mbi->common.framebuffer_addr;
                        fb->width = mbi->common.framebuffer_width;
                        fb->height = mbi->common.framebuffer_height;
                        fb->pitch = mbi->common.framebuffer_pitch;
                        fb->bpp = mbi->common.framebuffer_bpp;

                        fb->redShift = mbi->framebuffer_red_field_position;
                        fb->redBits = mbi->framebuffer_red_mask_size;
                        fb->greenShift = mbi->framebuffer_green_field_position;
                        fb->greenBits = mbi->framebuffer_green_mask_size;
                        fb->blueShift = mbi->framebuffer_blue_field_position;
                        fb->blueBits = mbi->framebuffer_blue_mask_size;

                        g_bootInfo.frameBufferCount = 1;
                        g_bootInfo.framebuffers = (uintptr_t)fb;
                    }
                    break;

                case MULTIBOOT2_FRAMEBUFFER_TYPE_EGA_TEXT:
                    {
                        g_vgaConsole.Initialize((void*)mbi->common.framebuffer_addr, mbi->common.framebuffer_width, mbi->common.framebuffer_height);
                        g_console = &g_vgaConsole;

                        FrameBufferInfo* fb = &g_frameBuffer;

                        fb->type = FrameBufferType_VGAText;
                        fb->address = mbi->common.framebuffer_addr;
                        fb->width = mbi->common.framebuffer_width;
                        fb->height = mbi->common.framebuffer_height;
                        fb->pitch = mbi->common.framebuffer_pitch;
                        fb->bpp = mbi->common.framebuffer_bpp;

                        g_bootInfo.frameBufferCount = 1;
                        g_bootInfo.framebuffers = (uintptr_t)fb;
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
    //*(uint16_t*)0x000B8000 = 0x5757;
    //for(;;);

    memset(&g_bootInfo, 0, sizeof(g_bootInfo));
    g_bootInfo.version = RAINBOW_BOOT_VERSION;
    g_bootInfo.firmware = Firmware_BIOS;

    // Add bootloader (ourself) to memory map
    extern const char bootloader_image_start[];
    extern const char bootloader_image_end[];

    const physaddr_t start = (physaddr_t)&bootloader_image_start;
    const physaddr_t end = (physaddr_t)&bootloader_image_end;
    g_memoryMap.AddBytes(MemoryType_Bootloader, 0, start, end - start);

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
