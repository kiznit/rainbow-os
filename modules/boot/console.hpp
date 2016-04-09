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

#ifndef _RAINBOW_BOOT_CONSOLE_HPP
#define _RAINBOW_BOOT_CONSOLE_HPP

#include <rainbow/types.h>


class Console
{
public:

    // Output
    virtual int GetChar();
    virtual int PutChar(int c);
    virtual int Print(const char* string, size_t length);

    // Cursor
    virtual void SetCursorPosition(int, int) {}
    virtual void ShowCursor() {}
    virtual void HideCursor() {}

    // Misc
    virtual void Clear() {}
    virtual void Scroll() {}
    virtual void Rainbow() {}
};




class VgaTextConsole : public Console
{
public:

    enum Color
    {
        Color_Black = 0,
        Color_Blue = 1,
        Color_Green = 2,
        Color_Cyan = 3,
        Color_Red = 4,
        Color_Magenta = 5,
        Color_Brown = 6,
        Color_LightGray = 7,
        Color_DarkGray = 8,
        Color_LightBlue = 9,
        Color_LightGreen = 10,
        Color_LightCyan = 11,
        Color_LightRed = 12,
        Color_LightMagenta = 13,
        Color_Yellow = 14,
        Color_White = 15,
    };

    VgaTextConsole(void* address, int width, int height);

    void SetColors(Color foregroundColor, Color backgroundColor)
    {
        m_colors = MakeColors(foregroundColor, backgroundColor);
    }

    // Console overrides
    virtual void Clear();
    virtual void HideCursor();
    virtual int PutChar(int c);
    virtual void Rainbow();
    virtual void Scroll();
    virtual void SetCursorPosition(int x, int y);
    virtual void ShowCursor();


private:

    // Helpers
    static inline int MakeColors(Color foregroundColor, Color backgroundColor)
    {
        return foregroundColor | backgroundColor << 4;
    }


    static inline uint16_t MakeChar(char c, uint8_t color)
    {
        uint16_t c16 = c;
        uint16_t color16 = color;
        return c16 | color16 << 8;
    }


    // Data
    uint16_t* const m_framebuffer;
    const int       m_width;
    const int       m_height;

    int             m_cursorX;
    int             m_cursorY;
    int             m_colors;
    bool            m_cursorVisible;
};


#endif
