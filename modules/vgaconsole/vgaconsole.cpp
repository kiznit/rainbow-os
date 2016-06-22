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
#include <arch/io.hpp>
#include <stdio.h>



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
    SetColors(Color_LightGray, Color_Black);
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



int VgaConsole::Print(const char* string, size_t length)
{
    for (size_t i = 0; i != length; ++i)
    {
        PutChar(string[i]);
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



void VgaConsole::SetColors(Color foregroundColor, Color backgroundColor)
{
    m_colors = foregroundColor | (backgroundColor & 7) << 4;
}



void VgaConsole::SetColors(ForegroundColor foregroundColor, Color backgroundColor)
{
    m_colors = foregroundColor | (backgroundColor & 7) << 4;
}



void VgaConsole::SetCursorPosition(int x, int y)
{
    if (x < 0) x = 0; else if (x >= m_width) x = m_width - 1;
    if (y < 0) y = 0; else if (y >= m_height) y = m_height - 1;

    m_cursorX = x;
    m_cursorY = y;

    if (m_cursorVisible)
    {
       uint16_t cursorLocation = y * m_width + x;
       io_write8(0x3D4, 14);
       io_write8(0x3D5, cursorLocation >> 8);
       io_write8(0x3D4, 15);
       io_write8(0x3D5, cursorLocation);
    }
}
