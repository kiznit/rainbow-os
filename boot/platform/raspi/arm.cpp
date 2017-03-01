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
#include "memory.hpp"



static void ProcessAtags(const atag::Entry* atags, BootInfo* bootInfo, MemoryMap* memoryMap)
{
    const atag::Entry* entry;

    for (entry = atags; entry && entry->type != atag::ATAG_NONE; entry = advance_pointer(entry, entry->size * 4))
    {
        switch (entry->type)
        {
        case atag::ATAG_MEMORY:
            {
                const auto memory = static_cast<const atag::Memory*>(entry);
                memoryMap->AddBytes(MemoryType_Available, 0, memory->address, memory->size);
            }
            break;

        case atag::ATAG_INITRD2:
            {
                const auto initrd = static_cast<const atag::Initrd2*>(entry);
                bootInfo->initrdAddress = initrd->address;
                bootInfo->initrdSize = initrd->size;
            }
            break;
        }
    }

    // Add atags to memory map
    const uint64_t start = (uintptr_t)atags;
    const uint64_t size  = (uintptr_t)entry - start + 1;
    memoryMap->AddBytes(MemoryType_Bootloader, MemoryFlag_ReadOnly, start, size);
}



// ref:
//  https://chromium.googlesource.com/chromiumos/third_party/dtc/+/master/fdtdump.c

static const fdt::Node* FindNode(const fdt::DeviceTree* deviceTree, const fdt::Node* parent, const char* nodeName)
{
    (void)deviceTree;

    int depth = 0;

    for (const fdt::Entry* entry = parent; be32toh(entry->type) != fdt::FDT_END; )
    {
        switch (be32toh(entry->type))
        {
        case fdt::FDT_BEGIN_NODE:
            {
                const auto node = static_cast<const fdt::Node*>(entry);

                if (++depth == 2 && strcmp(node->name, nodeName) == 0)
                {
                    return node;
                }

                entry = advance_pointer(entry, 4 + strlen(node->name) + 1);
                entry = align_up(entry, 4);
            }
            break;

        case fdt::FDT_END_NODE:
            {
                if (--depth < 0)
                {
                    return nullptr;
                }

                entry = advance_pointer(entry, 4);
            }
            break;

        case fdt::FDT_PROPERTY:
            {
                const auto property = static_cast<const fdt::Property*>(entry);
                const auto size = be32toh(property->size);
                const auto value = property->value;
                entry = (const fdt::Entry*)advance_pointer(value, size);
                entry = align_up(entry, 4);
            }
            break;

        case fdt::FDT_NOP:
            {
                entry = advance_pointer(entry, 4);
            }
            break;
        }
    }

    return nullptr;
}



static const fdt::Property* FindProperty(const fdt::DeviceTree* deviceTree, const fdt::Node* parent, const char* propertyName)
{
    const auto stringTable = (const char*)deviceTree + be32toh(deviceTree->offsetStrings);
    int depth = 0;

    for (const fdt::Entry* entry = parent; be32toh(entry->type) != fdt::FDT_END; )
    {
        switch (be32toh(entry->type))
        {
        case fdt::FDT_BEGIN_NODE:
            {
                const auto node = static_cast<const fdt::Node*>(entry);

                ++depth;

                entry = advance_pointer(entry, 4 + strlen(node->name) + 1);
                entry = align_up(entry, 4);
            }
            break;

        case fdt::FDT_END_NODE:
            {
                if (--depth < 0)
                {
                    return nullptr;
                }

                entry = advance_pointer(entry, 4);
            }
            break;

        case fdt::FDT_PROPERTY:
            {
                const auto property = static_cast<const fdt::Property*>(entry);
                const auto name = stringTable + be32toh(property->offsetName);

                if (strcmp(name, propertyName) == 0)
                {
                    return property;
                }

                const auto size = be32toh(property->size);
                const auto value = property->value;
                entry = (const fdt::Entry*)advance_pointer(value, size);
                entry = align_up(entry, 4);
            }
            break;

        case fdt::FDT_NOP:
            {
                entry = advance_pointer(entry, 4);
            }
            break;
        }
    }

    return nullptr;
}


static void ProcessDeviceTree(const fdt::DeviceTree* deviceTree, BootInfo* bootInfo, MemoryMap* memoryMap)
{
    const int version = be32toh(deviceTree->version);
    if (version < 16 || version > 17)
    {
        printf("*** Unsupported device tree version: %d\n", version);
        return;
    }

    // Add device tree to memory map
    memoryMap->AddBytes(MemoryType_Bootloader, MemoryFlag_ReadOnly, (uintptr_t)deviceTree, be32toh(deviceTree->size));

    // Reserved memory
    for (auto memory = (const fdt::ReservedMemory*)((const char*)deviceTree + be32toh(deviceTree->offsetReservedMemory)); memory->size != 0; ++memory)
    {
        const uint64_t address = be64toh(memory->address);
        const uint64_t size = be64toh(memory->size);
        memoryMap->AddBytes(MemoryType_Reserved, 0, address, size);
    }


    // Find interesting stuff
    uint32_t addressCells = 2;      // Default, as per spec
    uint32_t sizeCells = 1;         // Default, as per spec
    uint64_t initrdStart = 0;
    uint64_t initrdEnd = 0;

    const auto root = (const fdt::Node*)((const char*)deviceTree + be32toh(deviceTree->offsetStructures));

    auto property = FindProperty(deviceTree, root, "#address-cells");
    if (property)
    {
        const uint32_t* p = (const uint32_t*)property->value;
        addressCells = be32toh(*p);
    }

    property = FindProperty(deviceTree, root, "#size-cells");
    if (property)
    {
        const uint32_t* p = (const uint32_t*)property->value;
        sizeCells = be32toh(*p);
    }

    property = FindProperty(deviceTree, root, "memreserve");
    if (property)
    {
        for (const char* p = property->value; p != property->value + be32toh(property->size); )
        {
            uint64_t start;
            uint64_t size;

            if (addressCells == 1)
            {
                start = be32toh(*(uint32_t*)p); p += 4;
            }
            else
            {
                start = be64toh(*(uint64_t*)p); p += 8;
            }

            if (sizeCells == 1)
            {
                size = be32toh(*(uint32_t*)p); p += 4;
            }
            else
            {
                size = be64toh(*(uint64_t*)p); p += 8;
            }

            memoryMap->AddBytes(MemoryType_Reserved, 0, start, size);
        }
    }


    const auto chosen = FindNode(deviceTree, root, "chosen");
    if (chosen)
    {
        property = FindProperty(deviceTree, chosen, "linux,initrd-start");
        if (property)
        {
            const char* p = property->value;
            if (addressCells == 1)
            {
                initrdStart = be32toh(*(uint32_t*)p);
            }
            else
            {
                initrdStart = be64toh(*(uint64_t*)p);
            }
        }

        property = FindProperty(deviceTree, chosen, "linux,initrd-end");
        if (property)
        {
            const char* p = property->value;
            if (addressCells == 1)
            {
                initrdEnd = be32toh(*(uint32_t*)p);
            }
            else
            {
                initrdEnd = be64toh(*(uint64_t*)p);
            }
        }
    }

    if (initrdStart && initrdEnd > initrdStart)
    {
        bootInfo->initrdAddress = initrdStart;
        bootInfo->initrdSize = initrdEnd - initrdStart;
    }


    const auto memory = FindNode(deviceTree, root, "memory");
    if (memory)
    {
        property = FindProperty(deviceTree, memory, "reg");
        if (property)
        {
            for (const char* p = property->value; p != property->value + be32toh(property->size); )
            {
                uint64_t start;
                uint64_t size;

                if (addressCells == 1)
                {
                    start = be32toh(*(uint32_t*)p); p += 4;
                }
                else
                {
                    start = be64toh(*(uint64_t*)p); p += 8;
                }

                if (sizeCells == 1)
                {
                    size = be32toh(*(uint32_t*)p); p += 4;
                }
                else
                {
                    size = be64toh(*(uint64_t*)p); p += 8;
                }

                memoryMap->AddBytes(MemoryType_Available, 0, start, size);
            }
        }
    }
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
