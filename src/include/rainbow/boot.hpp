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
#include <metal/memory.hpp>
#include <graphics/pixels.hpp>
#include <rainbow/bitmask.hpp>


// The order these memory types are defined is important!
// When the firmware returns overlapping memory ranges, higher values take precedence.
enum class MemoryType
{
    // Normal memory (RAM) available for use.
    Available,

    // Normal memory (RAM) that contains errors and is not to be used.
    Unusable,

    // Normal memory (RAM) in use by the bootloader.
    // This memory can be reclaimed once the kernel is done reading bootloader data.
    Bootloader,

    // Normal memory (RAM) in use by the kernel.
    // This memory is/will be managed by the kernel.
    Kernel,

    // ACPI Tables (RAM).
    // The memory is to be preserved bu the OS until ACPI is enabled.
    // Once ACPI is enabled, the memory in this range is available for general use.
    AcpiReclaimable,

    // ACPI Non-Volatile Storage (RAM).
    // This memory is reserved for use by the firmware.
    // This memory needs to be preserved by the OS in ACPI S1-S3 states.
    AcpiNvs,

    // UEFI Runtime Services code and data (RAM).
    // This memory needs to be preserved by the OS in ACPI S1-S3 states.
    UefiCode,
    UefiData,

    // Works like normal memory, but is persistent (not RAM).
    Persistent,

    // Reserved / unknown / not usable / do not use (not RAM).
    Reserved,
};


enum class MemoryFlags : uint32_t
{
    None    = 0,

    // The following flags indicates capabilities, not configuration.
    // The values of the following flags match UEFI Memory Descriptor Attributes.
    UC      = 0x00000001,   // Uncacheable
    WC      = 0x00000002,   // Write Combining
    WT      = 0x00000004,   // Write-through
    WB      = 0x00000008,   // Writeback
    WP      = 0x00001000,   // Write-protected
    NV      = 0x00008000,   // Non-volatile

    RUNTIME = 0x80000000,   // Firmware runtime (i.e. UEFI Runtime Services)
};

ENABLE_BITMASK_OPERATORS(MemoryFlags)


struct MemoryDescriptor
{
    MemoryType  type;       // Memory type
    MemoryFlags flags;      // Flags
    physaddr_t  address;    // Start of memory range
    physaddr_t  size;       // Size of memory range in bytes
};


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
static_assert(sizeof(MemoryDescriptor) == 8 + 2*sizeof(physaddr_t));
static_assert(sizeof(Framebuffer) == 16 + sizeof(physaddr_t));
static_assert(sizeof(BootInfo) == 144 + 14*sizeof(physaddr_t));



#endif
