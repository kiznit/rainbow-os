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
#include <string.h>
#include <endian.h>


/*
    FDT = Flattened Device Tree

    refs:
        https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/plain/Documentation/devicetree/booting-without-of.txt
*/

namespace fdt {

const uint32_t FDT_BEGIN_NODE   = 1;    // Start of a node
const uint32_t FDT_END_NODE     = 2;    // End of a node
const uint32_t FDT_PROPERTY     = 3;    // Property
const uint32_t FDT_NOP          = 4;    // Nop
const uint32_t FDT_END          = 9;    // End of tree


struct Tag
{
    uint32_t type() const                   { return betoh32(type_); }
    inline const Tag* next(uint32_t version) const; // Pass in the device tree's version

    uint32_t type_;
};


struct NodeHeader : Tag
{
    char name[1];
};


struct Property : Tag
{
    uint32_t size() const                   { return betoh32(size_); }
    uint32_t offsetName() const             { return betoh32(offsetName_); }

    uint32_t size_;                 // Size of 'value' in bytes
    uint32_t offsetName_;           // Offset of name in string table
    char value[0];                  // Value, if any (aligned to 8 bytes if version < 16 && size >= 8)
};


struct Memory
{
    uint64_t address() const                { return betoh64(address_); }
    uint64_t size() const                   { return betoh64(size_); }

    uint64_t address_;
    uint64_t size_;
};



inline const Tag* Tag::next(uint32_t version) const
{
    switch (type())
    {
    case FDT_BEGIN_NODE:
        {
            auto header = static_cast<const NodeHeader*>(this);
            auto p = (uintptr_t)this + 4 + strlen(header->name) + 1;
            return (Tag*)((p + 3) & ~3);
        }
        break;

    case FDT_PROPERTY:
        {
            auto property = static_cast<const Property*>(this);
            auto size = property->size();
            auto p = (uintptr_t)this + 12;
            if (version < 16 && size >= 8)
            {
                p = (p + 7) & ~7;
            }
            return (Tag*)((p + size + 3) & ~3);
        }
        break;

    default:
        {
            return (Tag*)((uintptr_t)this + 4);
        }
        break;
    }
}



const uint32_t FDT_MAGIC = 0xd00dfeed;

struct DeviceTree
{
    uint32_t magic() const                  { return betoh32(magic_); }
    uint32_t size() const                   { return betoh32(size_); }
    const Tag* structures() const           { return (Tag*)((char*)this + betoh32(offsetStructures_)); }
    const char* strings() const             { return (char*)this + betoh32(offsetStrings_); }
    const Memory* reservedMemory() const    { return (Memory*)((char*)this + betoh32(offsetReservedMemory_)); }
    uint32_t version() const                { return betoh32(version_); }
    uint32_t lastCompatibleVersion() const  { return betoh32(lastCompatibleVersion_); }
    uint32_t bootCpuId() const              { return version() < 2 ? 0 : betoh32(bootCpuId_); }
    uint32_t sizeStrings() const            { return version() < 3 ? 0 : betoh32(sizeStrings_); }
    uint32_t sizesStructs() const           { return version() < 17 ? 0 : betoh32(sizesStructs_); }

    uint32_t magic_;                    // FDT_MAGIC
    uint32_t size_;                     // Size of Device Tree
    uint32_t offsetStructures_;         // Offset to structures
    uint32_t offsetStrings_;            // Offset to strings
    uint32_t offsetReservedMemory_;     // Offset to memory reserve map
    uint32_t version_;                  // Format version
    uint32_t lastCompatibleVersion_;    // Last compatible version

    // version 2 fields below
    uint32_t bootCpuId_;                // Boot CPU id

    // version 3 fields below
    uint32_t sizeStrings_;              // Size of the strings block

    // version 17 fields below
    uint32_t sizesStructs_;             // Size of the structures block
};



} // namespace fdt

#endif
