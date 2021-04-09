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

#include "console.hpp"
#include <cstring>
#include <new>
#include <type_traits>
#include "kernel.hpp"
#include "config.hpp"
#include "spinlock.hpp"
#include "vmm.hpp"
#include <graphics/graphicsconsole.hpp>
#include <graphics/surface.hpp>
#include <metal/helpers.hpp>
#include <metal/log.hpp>
#include <rainbow/boot.hpp>


// TODO: this static limit is not going to work if we get any processor with localApicId > 7.
const int MAX_CONSOLES = 8;

typedef std::aligned_storage<sizeof(GraphicsConsole), alignof(GraphicsConsole)>::type ConsoleStorage;
static ConsoleStorage s_console_storage[MAX_CONSOLES];
static GraphicsConsole* const s_console((GraphicsConsole*)&s_console_storage[0]);

static Surface s_framebuffer[MAX_CONSOLES];
static Surface s_backbuffer[MAX_CONSOLES];
static Spinlock s_spinlock;
static bool s_smp = false;

static const uint32_t s_colors[MAX_CONSOLES] =
{
    0x00000000,
    0x00000040,
    0x00004000,
    0x00400000,
    0x00004040,
    0x00400040,
    0x00404000,
    0x00404040
};


void console_early_init(Framebuffer* fb)
{
    // Manual s_console initialization because we need this to
    // happen before we initialize globals by calling constructors.
    // Logging and consoles need major refactoring anyways, so lets
    // address it later.
    for (int i = 0; i != MAX_CONSOLES; ++i)
    {
        new (&s_console_storage[i]) GraphicsConsole();
    }

    s_framebuffer[0].width = fb->width;
    s_framebuffer[0].height = fb->height;
    s_framebuffer[0].pitch = fb->pitch;
    s_framebuffer[0].pixels = VMA_FRAMEBUFFER_START;
    s_framebuffer[0].format = fb->format;

    s_console[0].Initialize(&s_framebuffer[0], &s_framebuffer[0]);
    s_console[0].Clear();

    s_console[0].Rainbow();
    const char* banner = " Kernel (" STRINGIZE(ARCH) ")\n\n";
    s_console[0].Print(banner, strlen(banner));
}


void console_init()
{
    const auto cpuCount = g_cpuCount;

    // Split the screen into multiple consoles (one per CPU)
    if (cpuCount > 1)
    {
        s_smp = true;

        // Copy framebuffer info
        for (auto i = 1; i != cpuCount; ++i)
        {
            s_framebuffer[i] = s_framebuffer[0];
        }

        // Calculate rows/columns
        const auto rows = cpuCount > 2 ? 2 : 1;
        const auto columns = (cpuCount + rows - 1) / rows;
        const auto width = s_framebuffer[0].width / columns;
        const auto height = s_framebuffer[0].height / rows;

        // Setup consoles
        for (auto j = 0; j != rows; ++j)
        {
            for (auto i = 0; i != columns; ++i)
            {
                const int index = i + j * columns;

                // Setup the console frontbuffer as a subset of the real frontbuffer
                Surface& framebuffer = s_framebuffer[index];
                framebuffer.width = width;
                framebuffer.height = height;
                framebuffer.pixels = advance_pointer(framebuffer.pixels, j * height * framebuffer.pitch + i * width * 4); // TODO: hardcoded 4 bytes/pixel...

                // Setup a console backbuffer
                Surface& backbuffer = s_backbuffer[index];
                backbuffer.width = width;
                backbuffer.height = height;
                backbuffer.pitch = width * 4;
                backbuffer.format = PixelFormat::X8R8G8B8;
                backbuffer.pixels = vmm_allocate_pages((backbuffer.pitch * backbuffer.height + MEMORY_PAGE_SIZE - 1) >> MEMORY_PAGE_SHIFT);
                // TODO: error handling if vmm_allocate_pages() failed above
                s_console[index].Initialize(&framebuffer, &backbuffer);
                s_console[index].SetBackgroundColor(s_colors[index]);
                s_console[index].Clear();
            }
        }
    }
    else
    {
        // Enable double buffering
        s_backbuffer[0].width = s_framebuffer[0].width;
        s_backbuffer[0].height = s_framebuffer[0].height;
        s_backbuffer[0].pitch = s_framebuffer[0].width * 4;
        s_backbuffer[0].format = PixelFormat::X8R8G8B8;
        s_backbuffer[0].pixels = vmm_allocate_pages((s_backbuffer[0].pitch * s_backbuffer[0].height + MEMORY_PAGE_SIZE - 1) >> MEMORY_PAGE_SHIFT);
        // TODO: error handling if vmm_allocate_pages() failed above
        s_console[0].Initialize(&s_framebuffer[0], &s_backbuffer[0]);
    }
}


void console_print(const char* text, size_t length)
{
    // In SMP, multiple processors could be trying to write to the same console. We don't want that.
    const bool needSpinlock = g_cpuCount > 1 && !s_smp;

    if (needSpinlock)
    {
        s_spinlock.lock();
    }

    const auto index = s_smp ? cpu_get_data(id) : 0;
    s_console[index].Print(text, length);

    if (needSpinlock)
    {
        s_spinlock.unlock();
    }
}
