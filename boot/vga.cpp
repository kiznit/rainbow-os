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

#include "vga.hpp"
#include <rainbow/io.h>
#include <stdio.h>


static const uint32_t s_vga_colors[16] =
{
    COLOR_VGA_BLACK,
    COLOR_VGA_BLUE,
    COLOR_VGA_GREEN,
    COLOR_VGA_CYAN,
    COLOR_VGA_RED,
    COLOR_VGA_MAGENTA,
    COLOR_VGA_BROWN,
    COLOR_VGA_LIGHT_GRAY,
    COLOR_VGA_DARK_GRAY,
    COLOR_VGA_LIGHT_BLUE,
    COLOR_VGA_LIGHT_GREEN,
    COLOR_VGA_LIGHT_CYAN,
    COLOR_VGA_LIGHT_RED,
    COLOR_VGA_LIGHT_MAGENTA,
    COLOR_VGA_YELLOW,
    COLOR_VGA_WHITE
};



static inline uint16_t VgaMakeChar(char c, uint8_t colors)
{
    uint16_t char16 = c;
    uint16_t colors16 = colors;
    return char16 | colors16 << 8;
}


void VgaTextOutput::Initialize(void* address, int width, int height)
{
    m_framebuffer = (uint16_t*)address;
    m_width = width;
    m_height = height;
    m_cursorX = 0;
    m_cursorY = 0;
    m_cursorVisible = true;

    EnableCursor(false);
    SetColors(COLOR_VGA_LIGHT_GRAY, COLOR_VGA_BLACK);
    Clear();
}



void VgaTextOutput::Clear()
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



void VgaTextOutput::EnableCursor(bool visible)
{
    if (visible)
    {
        // Solid block cursor
        outb(0x3d4, 0x0a);
        outb(0x3d5, 0x00);
    }
    else
    {
        // Hide cursor
        outw(0x3d4, 0x200a);
        outw(0x3d4, 0x000b);
    }

    m_cursorVisible = visible;
}



int VgaTextOutput::PutChar(int c)
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



void VgaTextOutput::Scroll()
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



void VgaTextOutput::SetColors(uint32_t foregroundColor, uint32_t backgroundColor)
{
    int fg = FindNearestColor(foregroundColor, s_vga_colors, 16);
    int bg = FindNearestColor(backgroundColor, s_vga_colors, 8);

    m_colors = fg | bg << 4;
}



void VgaTextOutput::SetCursorPosition(int x, int y)
{
    if (x < 0) x = 0; else if (x >= m_width) x = m_width - 1;
    if (y < 0) y = 0; else if (y >= m_height) y = m_height - 1;

    m_cursorX = x;
    m_cursorY = y;

    if (m_cursorVisible)
    {
       uint16_t cursorLocation = y * m_width + x;
       outb(0x3D4, 14);
       outb(0x3D5, cursorLocation >> 8);
       outb(0x3D4, 15);
       outb(0x3D5, cursorLocation);
    }
}



void VgaTextOutput::Rainbow()
{
    m_colors = 4;   PutChar('R');
    m_colors = 12;  PutChar('a');
    m_colors = 14;  PutChar('i');
    m_colors = 10;  PutChar('n');
    m_colors = 11;  PutChar('b');
    m_colors = 9;   PutChar('o');
    m_colors = 13;  PutChar('w');
    m_colors = 7;   PutChar(' ');
}
