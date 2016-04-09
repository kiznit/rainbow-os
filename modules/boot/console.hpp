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

#ifndef _RAINBOW_BOOT_CONSOLE_HPP
#define _RAINBOW_BOOT_CONSOLE_HPP

#include <rainbow/types.h>



class IConsoleTextInput
{
public:

    // Blocking call to read a key press, works just like libc's getchar()
    virtual int GetChar();
};



class IConsoleTextOutput
{
public:

    // CGA/EGA/VGA Colors =)
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

    // Output a character on the screen, works just like libc's putchar()
    virtual int PutChar(int c);

    // Ouput 'length' characters from 'string'
    virtual int Print(const char* string, size_t length);

    // Change text color attributes
    virtual void SetColors(Color foregroundColor, Color backgroundColor);

    // Clear the screem
    virtual void Clear();

    // Show / hide the cursor
    virtual void EnableCursor(bool visible);

    // Move the cursor to the specified position
    virtual void SetCursorPosition(int x, int y);

    // Display "Rainbow" in colors
    virtual void Rainbow();
};




#endif
