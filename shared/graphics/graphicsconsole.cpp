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
#include "boot.hpp"
#include "surface.hpp"
#include "vgafont.hpp"

#include "log.hpp"


void GraphicsConsole::Initialize(Surface* frontBuffer)
{
    m_frontBuffer = frontBuffer;
    m_width = frontBuffer->width / 8;
    m_height = frontBuffer->height / 16;
    m_cursorX = 0;
    m_cursorY = 0;
    m_foregroundColor = 0x00AAAAAA;
    m_backgroundColor = 0x00000000;
}



void GraphicsConsole::Clear()
{
    for (int y = 0; y != m_frontBuffer->height; ++y)
    {
        uint32_t* dest = (uint32_t*)(((uintptr_t)m_frontBuffer->pixels) + y * m_frontBuffer->pitch);
        for (int i = 0; i != m_frontBuffer->width; ++i)
        {
            *dest++ = m_backgroundColor;
        }
    }
}



void GraphicsConsole::Print(const char* string)
{
    size_t length = 0;
    for (const char* p = string; *p; ++p, ++length)
    {
        PutChar(*p);
    }
}



void GraphicsConsole::Rainbow()
{
    // https://www.webnots.com/vibgyor-rainbow-color-codes/
    m_foregroundColor = 0xFF0000; PutChar('R');
    m_foregroundColor = 0xFF7F00; PutChar('a');
    m_foregroundColor = 0xFFFF00; PutChar('i');
    m_foregroundColor = 0x00FF00; PutChar('n');
    m_foregroundColor = 0x0000FF; PutChar('b');
    m_foregroundColor = 0x4B0082; PutChar('o');
    m_foregroundColor = 0x9400D3; PutChar('w');

    m_foregroundColor = 0xAAAAAA;
}



void GraphicsConsole::PutChar(int c)
{
    if (c == '\n')
    {
        m_cursorX = 0;
        m_cursorY += 1;
    }
    else
    {
        VgaPutChar(c, m_frontBuffer, m_cursorX * 8, m_cursorY * 16, m_foregroundColor, m_backgroundColor);

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
}



void GraphicsConsole::Scroll() const
{
    // Scroll text
    for (int y = 16; y != m_frontBuffer->height; ++y)
    {
        void* dest = (void*)(((uintptr_t)m_frontBuffer->pixels) + (y - 16) * m_frontBuffer->pitch);
        const void* src = (void*)(((uintptr_t)m_frontBuffer->pixels) + y * m_frontBuffer->pitch);
        memcpy(dest, src, m_frontBuffer->width * 4);
    }

    // Erase last line
    for (int y = m_frontBuffer->height - 16; y != m_frontBuffer->height; ++y)
    {
        uint32_t* dest = (uint32_t*)(((uintptr_t)m_frontBuffer->pixels) + y * m_frontBuffer->pitch);
        for (int i = 0; i != m_frontBuffer->width; ++i)
        {
            *dest++ = m_backgroundColor;
        }
    }
}



void GraphicsConsole::SetCursorPosition(int x, int y)
{
    if (x < 0) x = 0; else if (x >= m_width) x = m_width - 1;
    if (y < 0) y = 0; else if (y >= m_height) y = m_height - 1;

    m_cursorX = x;
    m_cursorY = y;
}
