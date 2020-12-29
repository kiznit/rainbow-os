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

#ifndef _RAINBOW_GRAPHICS_GRAPHICSCONSOLE_HPP
#define _RAINBOW_GRAPHICS_GRAPHICSCONSOLE_HPP

#include <stdint.h>
#include <metal/console.hpp>


class Surface;


class GraphicsConsole : public IConsole
{
public:

    // If you pass backbuffer == frontbuffer, it's fine, but you won't get any buffering.
    void Initialize(Surface* frontbuffer, Surface* backbuffer);

    // Clear the screen
    void Clear();

    // Write a character to the screen
    void PutChar(int c);

    // Set the cursor's location
    void SetCursorPosition(int x, int y);

    // Set the background color
    void SetBackgroundColor(uint32_t color) { m_backgroundColor = color; }

    // Write a string to the screen
    void Print(const char* string, size_t length) override;

    // Print "Rainbow" in colors
    void Rainbow() override;


private:

    // Blit backbuffer to front buffer
    void Blit();

    // Draw a char to the backbuffer
    void DrawChar(int c);

    // Scroll the screen up by one row
    void Scroll() const;

    Surface*    m_frontbuffer;
    Surface*    m_backbuffer;
    int         m_width;
    int         m_height;
    int         m_cursorX;
    int         m_cursorY;
    uint32_t    m_foregroundColor;
    uint32_t    m_backgroundColor;

    // Dirty rectangle for Blit()
    mutable int m_dirtyLeft;
    mutable int m_dirtyTop;
    mutable int m_dirtyRight;
    mutable int m_dirtyBottom;
};


#endif
