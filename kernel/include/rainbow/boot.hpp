/*
    Copyright (c) 2022, Thierry Tremblay
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

#pragma once

#include <cstdint>
#include <metal/arch.hpp>
#include <metal/graphics/PixelFormat.hpp>

using PhysicalAddress = mtl::PhysicalAddress;

static constexpr uint32_t kRainbowBootVersion = 1;

struct Framebuffer
{
    int width;
    int height;
    int pitch;
    mtl::PixelFormat format;
    PhysicalAddress pixels;
};

static_assert(sizeof(Framebuffer) == 16 + sizeof(PhysicalAddress));

struct Module
{
    PhysicalAddress address;
    PhysicalAddress size;
};

struct BootInfo
{
    uint32_t version;                // Version (kRainbowBootVersion)
    uint32_t memoryMapLength;        // Number of available memory descriptors
    PhysicalAddress memoryMap;       // Memory descriptors
    PhysicalAddress uefiSystemTable; // UEFI System Table
    Framebuffer framebuffer;         // Frame buffer (but not always be available!)
};

// Make sure the BootInfo structure layout and size is the same when compiling
// the bootloader and the kernel. Otherwise things will just not work.
static_assert(sizeof(BootInfo) == 8 + 2 * sizeof(PhysicalAddress) + sizeof(Framebuffer));
