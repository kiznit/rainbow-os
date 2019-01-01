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

#include <libk/libk.hpp>
#include <libk/log.hpp>
#include <graphics/graphicsconsole.hpp>
#include <graphics/surface.hpp>


static Surface g_frameBuffer;
static GraphicsConsole g_graphicsConsole;


int x[10];
int abc = 123;


static void InitConsole()
{
    // TODO: these params need to come from the bootloader
    // Right now these are harcoded for QEMU
    g_frameBuffer.width = 800;
    g_frameBuffer.height = 600;
    g_frameBuffer.pitch = 800 * 4;
    g_frameBuffer.pixels = (void*)0xC0000000;
    g_frameBuffer.format = PIXFMT_X8R8G8B8;

    g_graphicsConsole.Initialize(&g_frameBuffer);
    g_graphicsConsole.Clear();

    g_graphicsConsole.Rainbow();
    g_graphicsConsole.Print(" Kernel (" STRINGIZE(ARCH) ")\n\n");

    g_console = &g_graphicsConsole;
}


extern "C" const char* kernel_main()
{
    InitConsole();

    Log("Here is some information from the kernel\n");

    for(;;);

    return "Hello from the kernel!\n";
}
