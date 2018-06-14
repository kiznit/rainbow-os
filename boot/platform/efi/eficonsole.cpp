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

#include "eficonsole.hpp"
#include "boot.hpp"
#include <limits.h>


// Here I assume the palette is the same one as for VGA.
// I am really not convinced there is a universal palette
// for all EFI implementations.
const int efiColorPalette[16][3] =
{
    0x00, 0x00, 0x00,   // Black
    0x00, 0x00, 0xAA,   // Blue
    0x00, 0xAA, 0x00,   // Green
    0x00, 0xAA, 0xAA,   // Cyan
    0xAA, 0x00, 0x00,   // Red
    0xAA, 0x00, 0xAA,   // Magenta
    0xAA, 0x55, 0x00,   // Brown
    0xAA, 0xAA, 0xAA,   // LightGray
    0x55, 0x55, 0x55,   // DarkGray
    0x55, 0x55, 0xFF,   // LightBlue
    0x55, 0xFF, 0x55,   // LightGreen
    0x55, 0xFF, 0xFF,   // LightCyan
    0xFF, 0x55, 0x55,   // LightRed
    0xFF, 0x55, 0xFF,   // LightMagenta
    0xFF, 0xFF, 0x55,   // Yellow
    0xFF, 0xFF, 0xFF,   // White
};


// Find closest color
static int FindClosestEfiColor(uint32_t color, bool background)
{
    const int r = (color >> 16) & 0xFF;
    const int g = (color >> 8) & 0xFF;
    const int b = color & 0xFF;

    int result = 0;
    int bestDistance2 = INT_MAX;

    for (int i = 0; i != (background ? 8 : 16); ++i)
    {
        // Refs: https://www.compuphase.com/cmetric.htm
        const int rmean = (efiColorPalette[i][0] + r) / 2;
        const int dr = (efiColorPalette[i][0] - r);
        const int dg = (efiColorPalette[i][1] - g);
        const int db = (efiColorPalette[i][2] - b);
        const int distance2 = (((512+rmean)*dr*dr)>>8) + 4*dg*dg + (((767-rmean)*db*db)>>8);

        if (distance2 < bestDistance2)
        {
            bestDistance2 = distance2;
            result = i;
        }
    }

    return result;
}



void EfiConsole::Initialize(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* console)
{
    // Mode 0 is always 80x25 text mode and is always supported
    // Mode 1 is always 80x50 text mode and isn't always supported
    // Modes 2+ are differents on every device
    size_t mode = 0;
    size_t width = 80;
    size_t height = 25;

    for (size_t m = 0; ; ++m)
    {
        size_t  w, h;
        EFI_STATUS status = console->QueryMode(console, m, &w, &h);
        if (EFI_ERROR(status))
        {
            // Mode 1 might return EFI_UNSUPPORTED, we still want to check modes 2+
            if (m > 1)
                break;
        }
        else
        {
            if (w * h > width * height)
            {
                mode = m;
                width = w;
                height = h;
            }
        }
    }

    console->SetMode(console, mode);

    // Some firmware won't clear the screen and/or reset the text colors on SetMode().
    // This is presumably more likely to happen when the selected mode is the current one.
    console->SetAttribute(console, EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLACK));
    console->ClearScreen(console);
    console->EnableCursor(console, FALSE);
    console->SetCursorPosition(console, 0, 0);

    m_console = console;
}



void EfiConsole::Clear()
{
    m_console->ClearScreen(m_console);
}



void EfiConsole::EnableCursor(bool visible)
{
    m_console->EnableCursor(m_console, visible ? TRUE : FALSE);
}



int EfiConsole::Print(const char* string)
{
    //TODO: do we need these checks? It might be better to "shutdown" the console after calling ExitBootServices()?

    // if (!g_efiSystemTable)
    //     return EOF;

    // EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* console = g_efiSystemTable->ConOut;
    // if (!console)
    //     return EOF;

    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* const console = m_console;

    size_t length = 0;

    CHAR16 buffer[200];
    size_t count = 0;

    for (const char* p = string; *p; ++p, ++length)
    {
        const unsigned char c = *p;

        if (c == '\n')
            buffer[count++] = '\r';

        buffer[count++] = c;

        if (count >= ARRAY_LENGTH(buffer) - 3)
        {
            buffer[count] = '\0';
            console->OutputString(console, buffer);
            count = 0;
        }
    }

    if (count > 0)
    {
        buffer[count] = '\0';
        console->OutputString(console, buffer);
    }

    return length;
}



int EfiConsole::PutChar(int c)
{
    CHAR16 string[] = { (CHAR16)c, '\0' };
    m_console->OutputString(m_console, string);

    return (CHAR16)c;
}



void EfiConsole::Rainbow()
{
    SetColors(0xFF0000, 0); PutChar('R');
    SetColors(0xFF7F00, 0); PutChar('a');
    SetColors(0xFFFF00, 0); PutChar('i');
    SetColors(0x00FF00, 0); PutChar('n');
    SetColors(0x0000FF, 0); PutChar('b');
    SetColors(0x4B0082, 0); PutChar('o');
    SetColors(0x9400D3, 0); PutChar('w');

    SetColors(0xAAAAAA, 0);
}



void EfiConsole::SetColors(uint32_t foregroundColor, uint32_t backgroundColor)
{
    const int fg = FindClosestEfiColor(foregroundColor, false);
    const int bg = FindClosestEfiColor(backgroundColor, true);
    m_console->SetAttribute(m_console, EFI_TEXT_ATTR(fg, bg));
}



void EfiConsole::SetCursorPosition(int x, int y)
{
    m_console->SetCursorPosition(m_console, x, y);
}
