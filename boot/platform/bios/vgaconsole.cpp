/*
    Copyright (c) 2016, Thierry Tremblay
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

#include "vgaconsole.hpp"
#include <limits.h>


// Foreground and background colors
enum Color
{
    Color_Black,        // 000000
    Color_Blue,         // 0000AA
    Color_Green,        // 00AA00
    Color_Cyan,         // 00AAAA
    Color_Red,          // AA0000
    Color_Magenta,      // AA00AA
    Color_Brown,        // AA5500
    Color_LightGray,    // AAAAAA
};

// Foreground only colors
enum ForegroundColor
{
    Color_DarkGray = 8, // 555555
    Color_LightBlue,    // 5555FF
    Color_LightGreen,   // 55FF55
    Color_LightCyan,    // 55FFFF
    Color_LightRed,     // FF5555
    Color_LightMagenta, // FF55FF
    Color_Yellow,       // FFFF55
    Color_White         // FFFFFF
};


const int vgaColorPalette[16][3] =
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
static int FindClosestVgaColor(uint32_t color, bool background)
{
    const int r = (color >> 16) & 0xFF;
    const int g = (color >> 8) & 0xFF;
    const int b = color & 0xFF;

    int result = 0;
    int bestDistance2 = INT_MAX;

    for (int i = 0; i != (background ? 8 : 16); ++i)
    {
        // Refs: https://www.compuphase.com/cmetric.htm
        const int rmean = (vgaColorPalette[i][0] + r) / 2;
        const int dr = (vgaColorPalette[i][0] - r);
        const int dg = (vgaColorPalette[i][1] - g);
        const int db = (vgaColorPalette[i][2] - b);
        const int distance2 = (((512+rmean)*dr*dr)>>8) + 4*dg*dg + (((767-rmean)*db*db)>>8);

        if (distance2 < bestDistance2)
        {
            bestDistance2 = distance2;
            result = i;
        }
    }

    return result;
}



//#include <arch/io.hpp>
static inline void io_write8(uint16_t port, uint8_t value)
{
    asm volatile ("outb %1, %0" : : "dN" (port), "a" (value));
}

static inline void io_write16(uint16_t port, uint16_t value)
{
    asm volatile ("outw %1, %0" : : "dN" (port), "a" (value));
}



static inline uint16_t VgaMakeChar(char c, uint8_t colors)
{
    uint16_t char16 = c;
    uint16_t colors16 = colors;
    return char16 | colors16 << 8;
}



void VgaConsole::Initialize(void* framebuffer, int width, int height)
{
    m_framebuffer = (uint16_t*)framebuffer;
    m_width = width;
    m_height = height;
    m_cursorX = 0;
    m_cursorY = 0;
    m_cursorVisible = true;

    EnableCursor(false);
    m_colors = Color_LightGray;
    Clear();
}



void VgaConsole::Clear()
{
    const uint16_t c = VgaMakeChar(' ', m_colors);

    for (int y = 0; y != m_height; ++y)
    {
        for (int x = 0; x != m_width; ++x)
        {
            const int index = y * m_width + x;
            m_framebuffer[index] = c;
        }
    }
}



void VgaConsole::EnableCursor(bool visible)
{
    if (visible)
    {
        // Solid block cursor
        io_write8(0x3d4, 0x0a);
        io_write8(0x3d5, 0x00);
    }
    else
    {
        // Hide cursor
        io_write16(0x3d4, 0x200a);
        io_write16(0x3d4, 0x000b);
    }

    m_cursorVisible = visible;
}



int VgaConsole::PutChar(int c)
{
    if (c == '\n')
    {
        m_cursorX = 0;
        m_cursorY += 1;
    }
    else
    {
        const int index = m_cursorY * m_width + m_cursorX;

        m_framebuffer[index] = VgaMakeChar(c, m_colors);

        if (++m_cursorX == m_width)
        {
            m_cursorX = 0;
            m_cursorY += 1;
        }
    }

    if (m_cursorY == m_height)
    {
        Scroll();
        m_cursorY -= 1;
    }

    SetCursorPosition(m_cursorX, m_cursorY);

    return (unsigned char)c;
}



int VgaConsole::Print(const char* string)
{
    size_t length = 0;
    for (const char* p = string; *p; ++p, ++length)
    {
        PutChar(*p);
    }

    return length;
}



void VgaConsole::Rainbow()
{
    const int backupColors = m_colors;

    m_colors = Color_Red;          PutChar('R');
    m_colors = Color_LightRed;     PutChar('a');
    m_colors = Color_Yellow;       PutChar('i');
    m_colors = Color_LightGreen;   PutChar('n');
    m_colors = Color_LightCyan;    PutChar('b');
    m_colors = Color_LightBlue;    PutChar('o');
    m_colors = Color_LightMagenta; PutChar('w');

    m_colors = backupColors;
}



void VgaConsole::Scroll()
{
    // Can't use memcpy, some hardware is limited to 16 bits read/write
    int i;
    for (i = 0; i != m_width * (m_height - 1); ++i)
    {
        m_framebuffer[i] = m_framebuffer[i + m_width];
    }

    const uint16_t c = VgaMakeChar(' ', m_colors);
    for ( ; i != m_width * m_height; ++i)
    {
        m_framebuffer[i] = c;
    }
}



void VgaConsole::SetColors(uint32_t foregroundColor, uint32_t backgroundColor)
{
    m_colors = FindClosestVgaColor(foregroundColor, false) | FindClosestVgaColor(backgroundColor, true) << 4;
}



void VgaConsole::SetCursorPosition(int x, int y)
{
    if (x < 0) x = 0; else if (x >= m_width) x = m_width - 1;
    if (y < 0) y = 0; else if (y >= m_height) y = m_height - 1;

    m_cursorX = x;
    m_cursorY = y;

    if (m_cursorVisible)
    {
       const uint16_t cursorLocation = y * m_width + x;
       io_write8(0x3D4, 14);
       io_write8(0x3D5, cursorLocation >> 8);
       io_write8(0x3D4, 15);
       io_write8(0x3D5, cursorLocation);
    }
}
