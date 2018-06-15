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

#include "graphicsconsole.hpp"
#include <string.h>
#include "surface.hpp"
#include "vgafont.hpp"


void GraphicsConsole::Initialize(Surface* surface)
{
    m_surface = surface;
    m_width = surface->width / 8;
    m_height = surface->height / 16;
    m_cursorX = 0;
    m_cursorY = 0;
    m_foregroundColor = 0x00AAAAAA;
    m_backgroundColor = 0x00000000;
}



void GraphicsConsole::Clear()
{
    // We can only work with 32 bpp surfaces
    if (m_surface->format != PIXFMT_A8R8G8B8)
    {
        return;
    }

    for (int y = 0; y != m_surface->height; ++y)
    {
        uint32_t* dest = (uint32_t*)(((uintptr_t)m_surface->pixels) + y * m_surface->pitch);
        for (int i = 0; i != m_surface->width; ++i)
        {
            *dest++ = m_backgroundColor;
        }
    }
}



int GraphicsConsole::Print(const char* string)
{
    size_t length = 0;
    for (const char* p = string; *p; ++p, ++length)
    {
        PutChar(*p);
    }

    return length;
}



int GraphicsConsole::PutChar(int c)
{
    if (c == '\n')
    {
        m_cursorX = 0;
        m_cursorY += 1;
    }
    else
    {
        putchar(c, m_surface, m_cursorX * 8, m_cursorY * 16, m_foregroundColor, m_backgroundColor);

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



void GraphicsConsole::Rainbow()
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



void GraphicsConsole::SetColors(uint32_t foregroundColor, uint32_t backgroundColor)
{
    m_foregroundColor = foregroundColor;
    m_backgroundColor = backgroundColor;
}



void GraphicsConsole::EnableCursor(bool visible)
{
    // TODO: implement
    (void)visible;
}



void GraphicsConsole::SetCursorPosition(int x, int y)
{
    if (x < 0) x = 0; else if (x >= m_width) x = m_width - 1;
    if (y < 0) y = 0; else if (y >= m_height) y = m_height - 1;

    m_cursorX = x;
    m_cursorY = y;
}



void GraphicsConsole::Scroll()
{
    // We can only work with 32 bpp surfaces
    if (m_surface->format != PIXFMT_A8R8G8B8)
    {
        return;
    }

    // Scroll text
    for (int y = 16; y != m_surface->height; ++y)
    {
        void* dest = (void*)(((uintptr_t)m_surface->pixels) + (y - 16) * m_surface->pitch);
        const void* src = (void*)(((uintptr_t)m_surface->pixels) + y * m_surface->pitch);
        memcpy(dest, src, m_surface->width * 4);
    }

    // Erase last line
    for (int y = m_surface->height - 16; y != m_surface->height; ++y)
    {
        uint32_t* dest = (uint32_t*)(((uintptr_t)m_surface->pixels) + y * m_surface->pitch);
        for (int i = 0; i != m_surface->width; ++i)
        {
            *dest++ = m_backgroundColor;
        }
    }
}
