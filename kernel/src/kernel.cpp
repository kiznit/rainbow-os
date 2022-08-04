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

#include <metal/graphics/GraphicsConsole.hpp>
#include <metal/graphics/SimpleDisplay.hpp>
#include <metal/graphics/Surface.hpp>
#include <rainbow/boot.hpp>

static BootInfo g_bootInfo;

// TODO: temp stubs
extern "C" void* malloc(size_t)
{
    return nullptr;
}

extern "C" void free(void*)
{}

extern "C" void _kernel_start(const BootInfo& bootInfo)
{
    // Copy boot info into kernel space as we won't keep the original memory mapped for long.
    g_bootInfo = bootInfo;

    // TODO: Hacks to get graphics very early
    std::details::RefCounterObject<mtl::Surface> rco{g_bootInfo.framebuffer.width, g_bootInfo.framebuffer.height,
                                                     g_bootInfo.framebuffer.pitch, g_bootInfo.framebuffer.format,
                                                     (void*)g_bootInfo.framebuffer.pixels};
    ++rco.count; // Make sure we never actually free the surface

    auto frontbuffer = std::shared_ptr<mtl::Surface>(&rco);
    mtl::SimpleDisplay display(frontbuffer);

    mtl::GraphicsConsole console(&display);
    console.Clear();
    console.Print(u8"Hello, this is the kernel!");

    for (;;)
        ;
}
