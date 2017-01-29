
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

#ifndef INCLUDED_BOOT_MULTIBOOT_VGACONSOLE_HPP
#define INCLUDED_BOOT_MULTIBOOT_VGACONSOLE_HPP

#include <stddef.h>
#include <stdint.h>



class VgaConsole
{
public:

    // Foreground and background colors
    enum Color
    {
        Color_Black,
        Color_Blue,
        Color_Green,
        Color_Cyan,
        Color_Red,
        Color_Magenta,
        Color_Brown,
        Color_LightGray,
    };

    // Foreground only colors
    enum ForegroundColor
    {
        Color_DarkGray = 8,
        Color_LightBlue,
        Color_LightGreen,
        Color_LightCyan,
        Color_LightRed,
        Color_LightMagenta,
        Color_Yellow,
        Color_White
    };

    // Construction
    void Initialize(void* framebuffer, int width, int height);

    // Misc
    void Clear();
    void SetColors(Color foregroundColor, Color backgroundColor);
    void SetColors(ForegroundColor foregroundColor, Color backgroundColor);
    void Rainbow();

    // output
    int PutChar(int c);
    int Print(const char* string);
    int Print(const char* string, size_t length);

    // Cursor
    void EnableCursor(bool visible);
    void SetCursorPosition(int x, int y);



private:

    // Scroll the screen up by one row
    void Scroll();

    // Data
    uint16_t*   m_framebuffer;
    int         m_width;
    int         m_height;

    int         m_cursorX;
    int         m_cursorY;
    int         m_colors;
    bool        m_cursorVisible;
};


#endif
