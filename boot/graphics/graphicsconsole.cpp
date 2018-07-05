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
#include <stdlib.h>
#include <string.h>
#include "surface.hpp"
#include "vgafont.hpp"

void* operator new(size_t size) { return malloc(size); }


void GraphicsConsole::Initialize(Surface* frontBuffer)
{
    void* pixels = malloc(frontBuffer->width * frontBuffer->height * 4);
    memset(pixels, 0, frontBuffer->width * frontBuffer->height * 4);

    m_frontBuffer = frontBuffer;
    m_backBuffer = new Surface(frontBuffer->width, frontBuffer->height, frontBuffer->width * 4, pixels, PIXFMT_X8R8G8B8);
    m_width = frontBuffer->width / 8;
    m_height = frontBuffer->height / 16;
    m_cursorX = 0;
    m_cursorY = 0;
    m_foregroundColor = 0x00AAAAAA;
    m_backgroundColor = 0x00000000;
}



void GraphicsConsole::Clear()
{
    for (int y = 0; y != m_backBuffer->height; ++y)
    {
        uint32_t* dest = (uint32_t*)(((uintptr_t)m_backBuffer->pixels) + y * m_backBuffer->pitch);
        for (int i = 0; i != m_backBuffer->width; ++i)
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

    Flip();

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
        VgaPutChar(c, m_backBuffer, m_cursorX * 8, m_cursorY * 16, m_foregroundColor, m_backgroundColor);

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
        Flip();
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



void GraphicsConsole::Flip() const
{
    if (m_frontBuffer->format == m_backBuffer->format && m_frontBuffer->pitch == m_backBuffer->pitch)
    {
        memcpy(m_frontBuffer->pixels, m_backBuffer->pixels, m_backBuffer->height * m_backBuffer->pitch);
        return;
    }

    // We can only work with 32 bpp surfaces
    if (m_frontBuffer->format != PIXFMT_X8R8G8B8)
    {
        return;
    }

    for (int y = 0; y != m_backBuffer->height; ++y)
    {
        void* dest = (void*)(((uintptr_t)m_frontBuffer->pixels) + y * m_frontBuffer->pitch);
        const void* src = (void*)(((uintptr_t)m_backBuffer->pixels) + y * m_backBuffer->pitch);
        memcpy(dest, src, m_backBuffer->width * 4);
    }
}


void GraphicsConsole::Scroll() const
{
    // Scroll text
    for (int y = 16; y != m_backBuffer->height; ++y)
    {
        void* dest = (void*)(((uintptr_t)m_backBuffer->pixels) + (y - 16) * m_backBuffer->pitch);
        const void* src = (void*)(((uintptr_t)m_backBuffer->pixels) + y * m_backBuffer->pitch);
        memcpy(dest, src, m_backBuffer->width * 4);
    }

    // Erase last line
    for (int y = m_backBuffer->height - 16; y != m_backBuffer->height; ++y)
    {
        uint32_t* dest = (uint32_t*)(((uintptr_t)m_backBuffer->pixels) + y * m_backBuffer->pitch);
        for (int i = 0; i != m_backBuffer->width; ++i)
        {
            *dest++ = m_backgroundColor;
        }
    }
}
