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
#include "arm.hpp"
#include "boot.hpp"
#include "atags.hpp"
#include "fdt.hpp"




static void ProcessAtags(const atag::Entry* atags)
{
    printf("Found ATAGS at %p:\n", atags);

    for (const atag::Entry* entry = atags; entry && entry->type != atag::ATAG_NONE; entry = advance_pointer(entry, entry->size * 4))
    {
        switch (entry->type)
        {
        case atag::ATAG_CORE:
            if (entry->size > 2)
            {
                // My RaspberryPi 3 says flags = 0, pageSize = 0, rootDevice = 0. Mmm.
                auto core = static_cast<const atag::Core*>(entry);

                printf("    ATAG_CORE   : flags = 0x%08lx, pageSize = 0x%08lx, rootDevice = 0x%08lx\n", core->flags, core->pageSize, core->rootDevice);
            }
            else
            {
                printf("    ATAG_CORE   : no data\n");
            }
            break;

        case atag::ATAG_MEMORY:
            {
                // My RaspberryPi 3 has one entry: address 0, size 0x3b000000
                auto memory = static_cast<const atag::Memory*>(entry);
                printf("    ATAG_MEMORY : address = 0x%08lx, size = 0x%08lx\n", memory->address, memory->size);
            }
            break;

        case atag::ATAG_INITRD2:
            {
                // Works fine (that's good)
                auto initrd = static_cast<const atag::Initrd2*>(entry);
                printf("    ATAG_INITRD2: address = 0x%08lx, size = 0x%08lx\n", initrd->address, initrd->size);
                g_bootInfo.initrdAddress = initrd->address;
                g_bootInfo.initrdSize = initrd->size;
            }
            break;

        case atag::ATAG_CMDLINE:
            {
                // My RaspberryPi 3 has a lot to say:
                // "dma.dmachans=0x7f35 bcm2708_fb.fbwidth=656 bcm2708_fb.fbheight=416 bcm2709.boardrev=0xa22082 bcm2709.serial=0xe6aaac09 smsc95xx.macaddr=B8:27:EB:AA:AC:09
                //  bcm2708_fb.fbswap=1 bcm2709.uart_clock=48000000 vc_mem.mem_base=0x3dc00000 vc_mem.mem_size=0x3f000000  console=ttyS0,115200 kgdboc=ttyS0,115200 console=tty1
                //  root=/dev/mmcblk0p2 rootfstype=ext4 rootwait"
                auto commandLine = static_cast<const atag::CommandLine*>(entry);
                printf("    ATAG_CMDLINE: \"%s\"\n", commandLine->commandLine);
            }
            break;

        default:
            printf("    Unhandled ATAG: 0x%08lx\n", entry->type);
        }
    }
}



// ref:
//  https://chromium.googlesource.com/chromiumos/third_party/dtc/+/master/fdtdump.c

static void ProcessDeviceTree(const fdt::DeviceTree* deviceTree)
{
    printf("Device tree (%p):\n", deviceTree);
    printf("    size                    : 0x%08lx\n", betoh32(deviceTree->size));
    printf("    offsetStructures        : 0x%08lx\n", betoh32(deviceTree->offsetStructures));
    printf("    offsetStrings           : 0x%08lx\n", betoh32(deviceTree->offsetStrings));
    printf("    offsetReservedMemory    : 0x%08lx\n", betoh32(deviceTree->offsetReservedMemory));
    printf("    version                 : 0x%08lx\n", betoh32(deviceTree->version));
    printf("    lastCompatibleVersion   : 0x%08lx\n", betoh32(deviceTree->lastCompatibleVersion));
    printf("    bootCpuId               : 0x%08lx\n", betoh32(deviceTree->bootCpuId));
    printf("    sizeStrings             : 0x%08lx\n", betoh32(deviceTree->sizeStrings));
    printf("    sizesStructs            : 0x%08lx\n", betoh32(deviceTree->sizesStructs));

    //printf("\nReserved memory:\n");

    for (auto memory = (const fdt::ReservedMemory*)((const char*)deviceTree + betoh32(deviceTree->offsetReservedMemory)); memory->size != 0; ++memory)
    {
        uint64_t address = betoh64(memory->address);
        uint64_t size = betoh64(memory->size);
        (void)address;
        (void)size;
        //printf("    0x%016llx: 0x%016llx bytes\n", address, size);
    }

    //todo: make sure to add the device tree itself to the reserved memory map (it should be but isn't)
//    const uint32_t version = betoh32(deviceTree->version);

    //printf("\nNodes:\n");

    int depth = 0;
    bool chosen = false;


    auto stringTable = (const char*)deviceTree + betoh32(deviceTree->offsetStrings);
    auto version = betoh32(deviceTree->version);

    uint32_t addressCells = 2;      // Default, as per spec
    uint32_t sizeCells = 1;         // Default, as per spec
    uint64_t initrdStart = 0;
    uint64_t initrdEnd = 0;

    for (auto entry = (const fdt::Entry*)((const char*)deviceTree + betoh32(deviceTree->offsetStructures)); betoh32(entry->type) != fdt::FDT_END; )
    {
        switch (betoh32(entry->type))
        {
        case fdt::FDT_BEGIN_NODE:
            {
                auto header = static_cast<const fdt::NodeHeader*>(entry);
                //printf("    %2d - %p - FDT_BEGIN_NODE: %s\n", depth, entry, header->name);

                if (depth == 1 && strcmp(header->name, "chosen") == 0)
                {
                    chosen = true;
                }

                ++depth;

                entry = advance_pointer(entry, 4 + strlen(header->name) + 1);
                entry = align_up(entry, 4);
            }
            break;

        case fdt::FDT_END_NODE:
            {
                chosen = false;
                --depth;
                //printf("    %2d - %p - FDT_END_NODE\n", depth, entry);

                entry = advance_pointer(entry, 4);
            }
            break;

        case fdt::FDT_PROPERTY:
            {
                //todo: handle version here, see https://chromium.googlesource.com/chromiumos/third_party/dtc/+/master/fdtdump.c
                auto property = static_cast<const fdt::Property*>(entry);
                auto name = stringTable + betoh32(property->offsetName);
                auto size = betoh32(property->size);
                auto value = property->value;

                if (version < 16 && size >= 8)
                {
                    value = align_up(value, 8);
                }

                //printf("    %2d - %p - FDT_PROPERTY: %s, size %ld\n", depth, entry, name, size);

                // Root node has some properties we care about
                if (depth == 1)
                {
                    if (strcmp(name, "#address-cells") == 0)
                    {
                        const uint32_t* p = (const uint32_t*)value;
                        addressCells = betoh32(*p);
                    }
                    else if (strcmp(name, "##size-cells") == 0)
                    {
                        const uint32_t* p = (const uint32_t*)value;
                        sizeCells = betoh32(*p);
                    }
                    else if (strcmp(name, "memreserve") == 0)
                    {
                        //todo: memreserve is really an array of ranges, and we must use #address-cells
                        // for extra fun: #address-cells isn't defined before memreserve!
                        // idea: store pointer and post-process this one
                        const uint32_t* p = (const uint32_t*)value;
                        uint32_t start = betoh32(p[0]);
                        uint32_t size = betoh32(p[1]);
                        (void)start;
                        (void)size;
                        //printf("    --> start 0x%08lx, size 0x%08lx\n", start, size);
                    }
                }
                else if (chosen)
                {
                    if (strcmp(name, "linux,initrd-start") == 0)
                    {
                        //todo: property could be 64 bits
                        const uint32_t* p = (const uint32_t*)value;
                        initrdStart = betoh32(*p);
                    }
                    else if (strcmp(name, "linux,initrd-end") == 0)
                    {
                        //todo: property could be 64 bits
                        const uint32_t* p = (const uint32_t*)value;
                        initrdEnd = betoh32(*p);
                    }
                }

                entry = (const fdt::Entry*)advance_pointer(value, size);
                entry = align_up(entry, 4);
            }
            break;

        case fdt::FDT_NOP:
            {
                //printf("    %2d - %p - FDT_NOP\n", depth, entry);

                entry = advance_pointer(entry, 4);
            }
            break;
        }
    }


    if (initrdStart && initrdEnd > initrdStart)
    {
        g_bootInfo.initrdAddress = initrdStart;
        g_bootInfo.initrdSize = initrdEnd - initrdStart;
    }

    (void)addressCells;
    (void)sizeCells;
}



bool ProcessBootParameters(const void* parameters)
{
    // My Raspberry Pi 3 doesn't pass in the atags address in 'parameters', but they sure are there at 0x100
    if (parameters == nullptr)
    {
        // Check if atags are available in the usual location (0x100)
        const atag::Entry* atags = reinterpret_cast<const atag::Entry*>(0x100);
        if (atags->type == atag::ATAG_CORE)
        {
            parameters = atags;
        }
    }

    // Check for flattened device tree (FDT) first
    const fdt::DeviceTree* deviceTree = reinterpret_cast<const fdt::DeviceTree*>(parameters);
    const atag::Entry* atags = reinterpret_cast<const atag::Entry*>(0x100);

    if (deviceTree && betoh32(deviceTree->magic) == fdt::FDT_MAGIC)
    {
        ProcessDeviceTree(deviceTree);
        return true;
    }
    else if (atags && atags->type == atag::ATAG_CORE)
    {
        ProcessAtags(atags);
        return true;
    }
    else
    {
        printf("WARNING: no boot parameters (atags or device tree) detected!\n");
        return false;
    }
}
