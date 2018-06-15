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

#ifndef BOOT_GRAPHICS_GRAPHICSCONSOLE_HPP
#define BOOT_GRAPHICS_GRAPHICSCONSOLE_HPP

#include "../console.hpp"

class Surface;


class GraphicsConsole : public Console
{
public:

    // Construction
    void Initialize(Surface* surface);

    // Clear the screen
    virtual void Clear();

    // Write a string to the screen
    virtual int Print(const char* string);

    // Write a character to the screen
    virtual int PutChar(int c);

    // Print "Rainbow" in colors
    virtual void Rainbow();

    // Select text colors (32 bits RGB format)
    virtual void SetColors(uint32_t foregroundColor, uint32_t backgroundColor);

    // Cursor
    virtual void EnableCursor(bool visible);
    virtual void SetCursorPosition(int x, int y);


private:

    // Scroll the screen up by one row
    void Scroll();

    // Data
    Surface*    m_surface;
    int         m_width;
    int         m_height;
    int         m_cursorX;
    int         m_cursorY;
    uint32_t    m_foregroundColor;
    uint32_t    m_backgroundColor;
};


#endif
