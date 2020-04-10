/*
    Copyright (c) 2018, Thierry Tremblay
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

#include "kernel.hpp"
#include <graphics/graphicsconsole.hpp>
#include <graphics/surface.hpp>
#include <rainbow/boot.hpp>

IConsole* g_console;

static Surface g_framebuffer;
static GraphicsConsole g_graphicsConsole;


void console_init(Framebuffer* fb)
{
    g_framebuffer.width = fb->width;
    g_framebuffer.height = fb->height;
    g_framebuffer.pitch = fb->pitch;
#if defined(__i386__)
    // TODO: on ia32, we mapped the framebuffer to 0xE0000000 in the bootloader
    // The reason for this is that we have to ensure the framebuffer isn't in
    // kernel space (>= 0xF0000000). This should go away once we more console
    // rendering out of the kernel.
    g_framebuffer.pixels = (void*)0xE0000000;
#elif defined(__x86_64__)
    // TODO: on x86_64, we mapped the framebuffer to 0xFFFF800000000000 in the bootloader
    // The reason for this is that we have to ensure the framebuffer isn't in
    // user space. This should go away once we more console rendering out of the kernel.
    g_framebuffer.pixels = (void*)0xFFFF800000000000;
#else
    g_framebuffer.pixels = (void*)fb->pixels;
#endif
    g_framebuffer.format = fb->format;

    g_graphicsConsole.Initialize(&g_framebuffer);
    g_graphicsConsole.Clear();

    g_graphicsConsole.Rainbow();
    g_graphicsConsole.Print(" Kernel (" STRINGIZE(ARCH) ")\n\n");

    g_console = &g_graphicsConsole;
}


void console_print(const char* text)
{
    // During kernel intitialization, we might not have initialized interrupts yet.
    // In that case, enabling interrupts crashes the kernel. We don't want that.
    const auto enableInterrupts = interrupt_enabled();

    if (enableInterrupts)
    {
        interrupt_disable();
    }

    g_console->Print(text);

    if (enableInterrupts)
    {
        interrupt_enable();
    }
}
