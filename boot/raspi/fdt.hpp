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
        https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/plain/Documentation/devicetree/booting-without-of.txt
*/

// Device tree header
#define FDT_HEADER 0xd00dfeed

struct fdt_header
{
    uint32_t magic;             // FDT_HEADER
    uint32_t totalsize;         // Size of DT block
    uint32_t off_dt_struct;     // Offset to structures
    uint32_t off_dt_strings;    // Offset to strings
    uint32_t off_mem_rsvmap;    // Offset to memory reserve map
    uint32_t version;           // Format version
    uint32_t last_comp_version; // Last compatible version

    // version 2 fields below
    uint32_t boot_cpuid_phys;   // Boot CPU id

    // version 3 fields below
    uint32_t size_dt_strings;   // Size of the strings block

    // version 17 fields below
    uint32_t size_dt_struct;    // Size of the structures block
};



// Defines reserved memory ranges
struct fdt_reserve_entry
{
    uint64_t address;
    uint64_t size;
};


// Tags
#define FDT_BEGIN_NODE 1
#define FDT_END_NODE 2
#define FDT_PROP 3
#define FDT_NOP 4
#define FDT_END 9

struct fdt_node_header
{
    uint32_t tag;
    char name[0];
};


struct fdt_property
{
    uint32_t tag;
    uint32_t len;
    uint32_t nameoff;
    char data[0];
};



#endif
