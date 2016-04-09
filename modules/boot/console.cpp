/*
    Copyright (c) 2015, Thierry Tremblay
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
#include <stdio.h>
#include <sys/io.h>



int Console::GetChar()
{
    return EOF;
}



int Console::PutChar(int c)
{
    char ch = c;
    Print(&ch, 1);
    return (unsigned char)c;
}



int Console::Print(const char* string, size_t length)
{
    for (size_t i = 0; i != length; ++i)
    {
        PutChar(string[i]);
    }

    return length;
}




VgaTextConsole::VgaTextConsole(void* address, int width, int height)
:   m_framebuffer((uint16_t*)address),
    m_width(width),
    m_height(height),
    m_cursorX(0),
    m_cursorY(0),
    m_colors(MakeColors(Color_LightGray, Color_Black)),
    m_cursorVisible(true)
{
    HideCursor();
    Clear();
}



void VgaTextConsole::Clear()
{
    const uint16_t c = MakeChar(' ', m_colors);

    for (int y = 0; y != m_height; ++y)
    {
        for (int x = 0; x != m_width; ++x)
        {
            const int index = y * m_width + x;
            m_framebuffer[index] = c;
        }
    }
}



void VgaTextConsole::HideCursor()
{
    outw(0x3d4, 0x200a);
    outw(0x3d4, 0x000b);
    m_cursorVisible = false;
}



int VgaTextConsole::PutChar(int c)
{
    if (c == '\n')
    {
        m_cursorX = 0;
        m_cursorY += 1;
    }
    else
    {
        const int index = m_cursorY * m_width + m_cursorX;

        m_framebuffer[index] = MakeChar(c, m_colors);

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



void VgaTextConsole::Rainbow()
{
    const uint16_t colors = m_colors;
    const Color backgroundColor = (Color)(colors >> 4);

    SetColors(Color_Red,        backgroundColor); PutChar('R');
    SetColors(Color_Brown,      backgroundColor); PutChar('a');
    SetColors(Color_Yellow,     backgroundColor); PutChar('i');
    SetColors(Color_LightGreen, backgroundColor); PutChar('n');
    SetColors(Color_Cyan,       backgroundColor); PutChar('b');
    SetColors(Color_LightBlue,  backgroundColor); PutChar('o');
    SetColors(Color_Magenta,    backgroundColor); PutChar('w');

    m_colors = colors;
    putchar(' ');
}



void VgaTextConsole::Scroll()
{
    // Can't use memcpy, some hardware is limited to 16 bits read/write
    int i;
    for (i = 0; i != m_width * (m_height - 1); ++i)
    {
        m_framebuffer[i] = m_framebuffer[i + m_width];
    }

    const uint16_t c = MakeChar(' ', m_colors);
    for ( ; i != m_width * m_height; ++i)
    {
        m_framebuffer[i] = c;
    }
}



void VgaTextConsole::SetCursorPosition(int x, int y)
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



void VgaTextConsole::ShowCursor()
{
    outb(0x3d4, 0x0a);
    outb(0x3d5, 0x00);
    m_cursorVisible = true;
}
