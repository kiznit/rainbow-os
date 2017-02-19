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

#include <endian.h>
#include <stdio.h>
#include <string.h>

#include "arm.hpp"
#include "boot.hpp"
#include "atags.hpp"
#include "fdt.hpp"
#include "../memory.hpp"



// static void HexDump(const char* start, const char* end)
// {
//     printf("\nHex Dump (%p - %p):\n", start, end);

//     int offset = 0;

//     for (const char* p = start; p != end; ++p, ++offset)
//     {
//         if (!(offset & 15))
//         {
//             printf("\n%08x:", offset);
//         }

//         printf(" %02x", *p);
//     }

//     printf("\n");
// }



static void ProcessAtags(const atag::Entry* atags, BootInfo* bootInfo, MemoryMap* memoryMap)
{
    for (const atag::Entry* entry = atags; entry && entry->type != atag::ATAG_NONE; entry = advance_pointer(entry, entry->size * 4))
    {
        switch (entry->type)
        {
        case atag::ATAG_MEMORY:
            {
                auto memory = static_cast<const atag::Memory*>(entry);
                memoryMap->AddBytes(MemoryType_Available, 0, memory->address, memory->size);
            }
            break;

        case atag::ATAG_INITRD2:
            {
                auto initrd = static_cast<const atag::Initrd2*>(entry);
                bootInfo->initrdAddress = initrd->address;
                bootInfo->initrdSize = initrd->size;
            }
            break;
        }
    }
}



// ref:
//  https://chromium.googlesource.com/chromiumos/third_party/dtc/+/master/fdtdump.c

static void ProcessDeviceTree(const fdt::DeviceTree* deviceTree, BootInfo* bootInfo, MemoryMap* memoryMap)
{
    //printf("Device tree (%p):\n", deviceTree);
    // printf("    size                    : 0x%08lx\n", be32toh(deviceTree->size));
    // printf("    offsetStructures        : 0x%08lx\n", be32toh(deviceTree->offsetStructures));
    // printf("    offsetStrings           : 0x%08lx\n", be32toh(deviceTree->offsetStrings));
    // printf("    offsetReservedMemory    : 0x%08lx\n", be32toh(deviceTree->offsetReservedMemory));
    // printf("    version                 : 0x%08lx\n", be32toh(deviceTree->version));
    // printf("    lastCompatibleVersion   : 0x%08lx\n", be32toh(deviceTree->lastCompatibleVersion));
    // printf("    bootCpuId               : 0x%08lx\n", be32toh(deviceTree->bootCpuId));
    // printf("    sizeStrings             : 0x%08lx\n", be32toh(deviceTree->sizeStrings));
    // printf("    sizesStructs            : 0x%08lx\n", be32toh(deviceTree->sizesStructs));

    //printf("\nReserved memory:\n");

    for (auto memory = (const fdt::ReservedMemory*)((const char*)deviceTree + be32toh(deviceTree->offsetReservedMemory)); memory->size != 0; ++memory)
    {
        uint64_t address = be64toh(memory->address);
        uint64_t size = be64toh(memory->size);
        (void)address;
        (void)size;
        //printf("    0x%016llx: 0x%016llx bytes\n", address, size);
    }

    //todo: make sure to add the device tree itself to the reserved memory map (it should be but isn't)
//    const uint32_t version = be32toh(deviceTree->version);

    //printf("\nNodes:\n");

    int depth = 0;
    bool chosen = false;


    auto stringTable = (const char*)deviceTree + be32toh(deviceTree->offsetStrings);
    auto version = be32toh(deviceTree->version);

    uint32_t addressCells = 2;      // Default, as per spec
    uint32_t sizeCells = 1;         // Default, as per spec
    uint64_t initrdStart = 0;
    uint64_t initrdEnd = 0;

    for (auto entry = (const fdt::Entry*)((const char*)deviceTree + be32toh(deviceTree->offsetStructures)); be32toh(entry->type) != fdt::FDT_END; )
    {
        switch (be32toh(entry->type))
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
                auto name = stringTable + be32toh(property->offsetName);
                auto size = be32toh(property->size);
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
                        addressCells = be32toh(*p);
                    }
                    else if (strcmp(name, "##size-cells") == 0)
                    {
                        const uint32_t* p = (const uint32_t*)value;
                        sizeCells = be32toh(*p);
                    }
                    else if (strcmp(name, "memreserve") == 0)
                    {
                        //todo: memreserve is really an array of ranges, and we must use #address-cells
                        // for extra fun: #address-cells isn't defined before memreserve!
                        // idea: store pointer and post-process this one
                        const uint32_t* p = (const uint32_t*)value;
                        uint32_t start = be32toh(p[0]);
                        uint32_t size = be32toh(p[1]);
                        memoryMap->AddBytes(MemoryType_Reserved, 0, start, size);
                        //printf("    --> start 0x%08lx, size 0x%08lx\n", start, size);
                    }
                }
                else if (chosen)
                {
                    if (strcmp(name, "linux,initrd-start") == 0)
                    {
                        //todo: property could be 64 bits
                        const uint32_t* p = (const uint32_t*)value;
                        initrdStart = be32toh(*p);
                    }
                    else if (strcmp(name, "linux,initrd-end") == 0)
                    {
                        //todo: property could be 64 bits
                        const uint32_t* p = (const uint32_t*)value;
                        initrdEnd = be32toh(*p);
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
        bootInfo->initrdAddress = initrdStart;
        bootInfo->initrdSize = initrdEnd - initrdStart;
    }

    (void)addressCells;
    (void)sizeCells;

    // const char* start = (char*)deviceTree;
    // const char* end = start + be32toh(deviceTree->size);
    // HexDump(start, end);
}



bool ProcessBootParameters(const void* parameters, BootInfo* bootInfo, MemoryMap* memoryMap)
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
    const atag::Entry* atags = reinterpret_cast<const atag::Entry*>(parameters);

    if (deviceTree && be32toh(deviceTree->magic) == fdt::FDT_MAGIC)
    {
        ProcessDeviceTree(deviceTree, bootInfo, memoryMap);
        return true;
    }
    else if (atags && atags->type == atag::ATAG_CORE)
    {
        ProcessAtags(atags, bootInfo, memoryMap);
        return true;
    }
    else
    {
        printf("WARNING: no boot parameters (atags or device tree) detected!\n");
        return false;
    }
}
