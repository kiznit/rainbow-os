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
#include "config.hpp"
#include "spinlock.hpp"
#include <graphics/graphicsconsole.hpp>
#include <graphics/surface.hpp>
#include <metal/helpers.hpp>
#include <metal/log.hpp>
#include <rainbow/boot.hpp>

#if defined(__i386__)
#include "x86/smp.hpp"
#include "x86/ia32/cpu.hpp"
#elif defined(__x86_64__)
#include "x86/smp.hpp"
#include "x86/x86_64/cpu.hpp"
#endif


static GraphicsConsole s_console[MAX_CPU];
static Surface s_framebuffer[MAX_CPU];
static Spinlock s_spinlock;
static bool s_smp = false;

static const uint32_t s_colors[MAX_CPU] =
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


void console_init(Framebuffer* fb)
{
    s_framebuffer[0].width = fb->width;
    s_framebuffer[0].height = fb->height;
    s_framebuffer[0].pitch = fb->pitch;
    s_framebuffer[0].pixels = VMA_FRAMEBUFFER_START;
    s_framebuffer[0].format = fb->format;

    s_console[0].Initialize(&s_framebuffer[0]);
    s_console[0].Clear();

    s_console[0].Rainbow();
    s_console[0].Print(" Kernel (" STRINGIZE(ARCH) ")\n\n");
}


void console_smp_init()
{
    Log("console_smp_init()\n");

    const auto cpuCount = g_cpuCount;

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
            Surface& surface = s_framebuffer[index];
            surface.width = width;
            surface.height = height;
            surface.pixels = advance_pointer(surface.pixels, j * height * surface.pitch + i * width * 4); // TODO: hardcoded 4 bytes/pixel...

            s_console[index].Initialize(&s_framebuffer[index]);
            s_console[index].SetBackgroundColor(s_colors[index]);
            s_console[index].Clear();
        }
    }

    s_smp = true;
}


void console_print(const char* text)
{
    // In SMP, multiple processors could be trying to write to the same console. We don't want that.
    const bool needSpinlock = g_cpuCount > 1 && !s_smp;

    if (needSpinlock)
    {
        s_spinlock.Lock();
    }

    const auto index = s_smp ? cpu_get_data(cpu)->apicId : 0;
    s_console[index].Print(text);

    if (needSpinlock)
    {
        s_spinlock.Unlock();
    }
}
