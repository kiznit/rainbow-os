/*
    Copyright (c) 2023, Thierry Tremblay
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

#include "memory.hpp"
#include <metal/graphics/GraphicsConsole.hpp>
#include <metal/graphics/SimpleDisplay.hpp>
#include <metal/graphics/Surface.hpp>
#include <metal/log.hpp>
#include <rainbow/boot.hpp>

void KernelMain(const BootInfo&);

using constructor_t = void();

extern constructor_t* __init_array_start[];
extern constructor_t* __init_array_end[];

extern "C" void __cxa_pure_virtual()
{
    assert(0 && "__cxa_pure_virtual()");
}

static BootInfo g_bootInfo;

static void Crt0CallGlobalConstructors()
{
    for (auto constructor = __init_array_start; constructor < __init_array_end; ++constructor)
    {
        (*constructor)();
    }
}

static void Crt0InitEarlyGraphicsConsole(const Framebuffer& framebuffer)
{
    auto frontbuffer = std::make_shared<mtl::Surface>(framebuffer.width, framebuffer.height, framebuffer.pitch, framebuffer.format,
                                                      (void*)framebuffer.pixels);

    auto backbuffer = std::make_shared<mtl::Surface>(framebuffer.width, framebuffer.height, mtl::PixelFormat::X8R8G8B8);
    memset(backbuffer->pixels, 0, backbuffer->height * backbuffer->pitch);

    auto display = std::make_shared<mtl::SimpleDisplay>(std::move(frontbuffer), std::move(backbuffer));
    auto console = std::make_shared<mtl::GraphicsConsole>(std::move(display));
    console->Clear();

    mtl::g_log.AddLogger(std::move(console));
}

extern "C" void _kernel_start(const BootInfo& bootInfo)
{
    Crt0CallGlobalConstructors();

    const auto descriptors = reinterpret_cast<const efi::MemoryDescriptor*>(bootInfo.memoryMap);
    MemoryEarlyInit(std::vector<efi::MemoryDescriptor>(descriptors, descriptors + bootInfo.memoryMapLength));

    if (bootInfo.framebuffer.pixels)
        Crt0InitEarlyGraphicsConsole(bootInfo.framebuffer);

    // Copy boot info into kernel space as we won't keep the original one memory mapped for long.
    g_bootInfo = bootInfo;

    return KernelMain(g_bootInfo);
}
