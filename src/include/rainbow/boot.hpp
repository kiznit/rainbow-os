/*
    Copyright (c) 2020, Thierry Tremblay
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

#ifndef _RAINBOW_BOOT_HPP
#define _RAINBOW_BOOT_HPP

#include <cstdint>
#include <metal/arch.hpp>
#include <graphics/pixels.hpp>


// The order these memory types are defined is important!
// When the firmware returns overlapping memory ranges, higher values take precedence.
enum class MemoryType
{
    Available,              // Conventional memory (RAM)
    Persistent,             // Works like conventional memory, but is persistent
    Unusable,               // Memory in which errors have been detected
    Bootloader,             // Bootloader
    Kernel,                 // Kernel
    AcpiReclaimable,        // ACPI Tables (can be reclaimed once parsed)
    AcpiNvs,                // ACPI Non-Volatile Storage
    Firmware,               // Firmware (e.g. EFI runtime services, ARM Device Tree, ...)
    Reserved,               // Reserved / unknown / do not use
};


struct MemoryDescriptor
{
    MemoryType  type;       // Memory type
    uint32_t    reserved;   // Reserved
    physaddr_t  address;    // Start of memory range
    physaddr_t  size;       // Size of memory range in bytes
};

static_assert(sizeof(MemoryDescriptor) == 24, "MemoryDescriptor should be packed to 24 bytes");



static const uint32_t RAINBOW_BOOT_VERSION = 1;


struct Framebuffer
{
    int         width;
    int         height;
    int         pitch;
    PixelFormat format;
    physaddr_t  pixels;
};


struct Module
{
    physaddr_t  address;
    physaddr_t  size;
};


struct BootInfo
{
    uint32_t            version;            // Version (RAINBOW_BOOT_VERSION)

    uint32_t            descriptorCount;    // Number of available memory descriptors
    physaddr_t          descriptors;        // Memory descriptors

    uint32_t            framebufferCount;   // Number of available displays
    uint32_t            padding;
    Framebuffer         framebuffers[8];    // Display frame buffers

    physaddr_t          acpiRsdp;           // ACPI Root System Descriptor Pointer

    Module              go;                 // go - bootstrap kernel services
    Module              logger;             // handle kernel logging
};

// Make sure the BootInfo structure layout and size is the same in both 32 and 64 bits mode.
// If this isn't the case, then booting a 64 bits kernel with a 32 bits bootloader won't work.
static_assert(sizeof(Framebuffer) == 24);
static_assert(sizeof(BootInfo) == 256);



#endif
