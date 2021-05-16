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

#include "graphicsconsole.hpp"

#include <algorithm>
#include <cstring>

#include "display.hpp"
#include "surface.hpp"
#include "vgafont.hpp"


void GraphicsConsole::Initialize(IDisplay* display)
{
    m_display = display;
    m_backbuffer = display->GetBackbuffer();
    m_width = m_backbuffer->width / 8;
    m_height = m_backbuffer->height / 16;
    m_cursorX = 0;
    m_cursorY = 0;
    m_foregroundColor = 0x00AAAAAA;
    m_backgroundColor = 0x00000000;

    // Set dirty rect to "nothing"
    m_dirtyLeft = m_backbuffer->width;
    m_dirtyTop = m_backbuffer->height;
    m_dirtyRight = 0;
    m_dirtyBottom = 0;
}


void GraphicsConsole::Blit()
{
    const int width = m_dirtyRight - m_dirtyLeft;
    const int height = m_dirtyBottom - m_dirtyTop;

    if (width <= 0 || height <= 0)
    {
        return;
    }

    m_display->Blit(m_dirtyLeft, m_dirtyTop, width, height);

    // Reset dirty rect to "nothing"
    m_dirtyLeft = m_backbuffer->width;
    m_dirtyTop = m_backbuffer->height;
    m_dirtyRight = 0;
    m_dirtyBottom = 0;
}


void GraphicsConsole::Clear()
{
    for (int y = 0; y != m_backbuffer->height; ++y)
    {
        uint32_t* dest = (uint32_t*)(((uintptr_t)m_backbuffer->pixels) + y * m_backbuffer->pitch);
        for (int i = 0; i != m_backbuffer->width; ++i)
        {
            *dest++ = m_backgroundColor;
        }
    }

    // Set dirty rect to the whole screen
    m_dirtyLeft = 0;
    m_dirtyTop = 0;
    m_dirtyRight = m_backbuffer->width;
    m_dirtyBottom = m_backbuffer->height;

    Blit();
}


void GraphicsConsole::DrawChar(int c)
{
    if (c == '\n')
    {
        m_cursorX = 0;
        m_cursorY += 1;
    }
    else
    {
        const auto px = m_cursorX * 8;
        const auto py = m_cursorY * 16;

        VgaDrawChar(c, m_backbuffer, px, py, m_foregroundColor, m_backgroundColor);

        // Update dirty rect
        m_dirtyLeft = std::min(px, m_dirtyLeft);
        m_dirtyTop = std::min(py, m_dirtyTop);
        m_dirtyRight = std::max(px + 8, m_dirtyRight);
        m_dirtyBottom = std::max(py + 16, m_dirtyBottom);

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


void GraphicsConsole::Print(const char* string, size_t length)
{
    for (const char* p = string; length; --length)
    {
        DrawChar(*p++);
    }

    Blit();
}


void GraphicsConsole::Rainbow()
{
    // https://www.webnots.com/vibgyor-rainbow-color-codes/
    m_foregroundColor = 0xFF0000; DrawChar('R');
    m_foregroundColor = 0xFF7F00; DrawChar('a');
    m_foregroundColor = 0xFFFF00; DrawChar('i');
    m_foregroundColor = 0x00FF00; DrawChar('n');
    m_foregroundColor = 0x0000FF; DrawChar('b');
    m_foregroundColor = 0x4B0082; DrawChar('o');
    m_foregroundColor = 0x9400D3; DrawChar('w');

    m_foregroundColor = 0xAAAAAA;
}


void GraphicsConsole::PutChar(int c)
{
    DrawChar(c);

    Blit();
}


void GraphicsConsole::Scroll() const
{
    // Scroll text
    for (int y = 16; y != m_backbuffer->height; ++y)
    {
        void* dest = (void*)(((uintptr_t)m_backbuffer->pixels) + (y - 16) * m_backbuffer->pitch);
        const void* src = (void*)(((uintptr_t)m_backbuffer->pixels) + y * m_backbuffer->pitch);
        memcpy(dest, src, m_backbuffer->width * 4);
    }

    // Erase last line
    for (int y = m_backbuffer->height - 16; y != m_backbuffer->height; ++y)
    {
        uint32_t* dest = (uint32_t*)(((uintptr_t)m_backbuffer->pixels) + y * m_backbuffer->pitch);

        if (m_backgroundColor == 0)
        {
            memset(dest, 0, m_backbuffer->width * 4);
        }
        else
        {
            for (int i = 0; i != m_backbuffer->width; ++i)
            {
                *dest++ = m_backgroundColor;
            }
        }
    }

    // Set dirty rect to the whole screen
    m_dirtyLeft = 0;
    m_dirtyTop = 0;
    m_dirtyRight = m_backbuffer->width;
    m_dirtyBottom = m_backbuffer->height;
}


void GraphicsConsole::SetCursorPosition(int x, int y)
{
    if (x < 0) x = 0; else if (x >= m_width) x = m_width - 1;
    if (y < 0) y = 0; else if (y >= m_height) y = m_height - 1;

    m_cursorX = x;
    m_cursorY = y;
}
