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

#ifndef BOOT_RASPI_ATAGS_HPP
#define BOOT_RASPI_ATAGS_HPP

#include <stdint.h>

/*
    ATAGS = ARM tags

    refs:
        http://www.simtec.co.uk/products/SWLINUX/files/booting_article.html
*/

namespace atag {

const uint32_t ATAG_NONE        = 0x00000000;   // Empty tag used to end list
const uint32_t ATAG_CORE        = 0x54410001;   // First tag used to start list
const uint32_t ATAG_MEMORY      = 0x54410002;   // Describes a physical area of memory
const uint32_t ATAG_VIDEOTEXT   = 0x54410003;   // Describes a VGA text display
const uint32_t ATAG_RAMDISK     = 0x54410004;   // Describes how the ramdisk will be used in kernel
const uint32_t ATAG_INITRD2     = 0x54420005;   // Describes where the compressed ramdisk image is placed in memory
const uint32_t ATAG_SERIAL      = 0x54410006;   // 64 bit board serial number
const uint32_t ATAG_REVISION    = 0x54410007;   // 32 bit board revision number
const uint32_t ATAG_VIDEOLFB    = 0x54410008;   // Initial values for vesafb-type framebuffers
const uint32_t ATAG_CMDLINE     = 0x54410009;   // Command line to pass to kernel

const uint32_t ATAG_ACORN       = 0x41000101;   // Acorn RiscPC specific information
const uint32_t ATAG_MEMCLK      = 0x41000402;   // Footbridge memory clock


struct Tag
{
    // Header
    uint32_t size;          // Length of tag in words, including this header
    uint32_t type;          // Tag type

    // Return the next tag
    const Tag* next() const
    {
        const Tag* tag = (Tag*)((uint32_t*)this + size);
        return tag->type == ATAG_NONE ? nullptr : tag;
    }
};


struct Core : Tag
{
    uint32_t flags;         // bit 0 = read-only
    uint32_t pageSize;      // systems page size (usually 4k)
    uint32_t rootDevice;    // root device number
};


struct Memory : Tag
{
    uint32_t size;          // size of the area
    uint32_t address;       // physical start address
};



// VGA text type displays
struct VideoText : Tag
{
    uint8_t  width;         // width of display
    uint8_t  height;        // height of display
    uint16_t page;
    uint8_t  mode;
    uint8_t  cols;
    uint16_t ega_bx;
    uint8_t  lines;
    uint8_t  isVGA;
    uint16_t points;
};


struct Ramdisk : Tag
{
    uint32_t flags;         // bit 0 = load, bit 1 = prompt
    uint32_t size;          // decompressed ramdisk size in _kilo_ bytes
    uint32_t start;         // starting block of floppy-based RAM disk image
};


struct Initrd2 : Tag
{
    uint32_t address;       // physical start address
    uint32_t size;          // size of compressed ramdisk image in bytes
};


struct SerialNumber : Tag
{
    uint32_t low;
    uint32_t high;
};


struct Revision : Tag
{
    uint32_t revision;
};


struct VideoFrameBuffer : Tag
{
    uint16_t width;
    uint16_t height;
    uint16_t bpp;
    uint16_t pitch;
    uint32_t address;
    uint32_t size;
    uint8_t  redBits;
    uint8_t  redShift;
    uint8_t  greenBits;
    uint8_t  greenShift;
    uint8_t  blueBits;
    uint8_t  blueShift;
    uint8_t  alphaBits;
    uint8_t  alphaShift;
};


struct CommandLine : Tag
{
    char commandLine[1];    // this is the minimum size
};


// Acorn specific
struct Acorn : Tag
{
    uint32_t memcControlRegister;
    uint32_t vramPages;
    uint8_t  soundDefault;
    uint8_t  adfsDrives;
};


// DC21275 specific
struct MemoryClock : Tag
{
    uint32_t frequency;
};



} // namespace atag

#endif
