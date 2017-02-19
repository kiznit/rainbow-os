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

#ifndef BOOT_RASPI_FDT_HPP
#define BOOT_RASPI_FDT_HPP

#include <stdint.h>



/*
    FDT = Flattened Device Tree

    refs:
        https://www.devicetree.org/
        https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/plain/Documentation/devicetree/booting-without-of.txt
*/

namespace fdt {

const uint32_t FDT_MAGIC = 0xd00dfeed;

struct DeviceTree
{
    uint32_t magic;                 // FDT_MAGIC
    uint32_t size;                  // Size of Device Tree
    uint32_t offsetStructures;      // Offset to structures
    uint32_t offsetStrings;         // Offset to strings
    uint32_t offsetReservedMemory;  // Offset to memory reserve map
    uint32_t version;               // Format version
    uint32_t lastCompatibleVersion; // Last compatible version

    // version 2 fields below
    uint32_t bootCpuId;             // Boot CPU id

    // version 3 fields below
    uint32_t sizeStrings;           // Size of the strings block

    // version 17 fields below
    uint32_t sizesStructs;          // Size of the structures block
};



enum Tag
{
    FDT_BEGIN_NODE  = 1,    // Start of a node
    FDT_END_NODE    = 2,    // End of a node
    FDT_PROPERTY    = 3,    // Property
    FDT_NOP         = 4,    // Nop
    FDT_END         = 9,    // End of tree
};



struct Entry
{
    uint32_t type;          // Tag
};


struct NodeHeader : Entry
{
    char name[1];
};


struct Property : Entry
{
    uint32_t size;          // Size of 'value' in bytes
    uint32_t offsetName;    // Offset of name in string table
    char value[0];          // Value, if any
};


struct ReservedMemory
{
    uint64_t address;
    uint64_t size;
};



} // namespace fdt

#endif
