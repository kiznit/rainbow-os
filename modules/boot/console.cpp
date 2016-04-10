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

#include "console.hpp"
#include <stdio.h>



int IConsoleTextInput::GetChar()
{
    return EOF;
}



int IConsoleTextOutput::PutChar(int c)
{
    char ch = c;
    Print(&ch, 1);
    return (unsigned char)ch;
}



int IConsoleTextOutput::Print(const char* string, size_t length)
{
    for (size_t i = 0; i != length; ++i)
    {
        PutChar(string[i]);
    }

    return length;
}



void IConsoleTextOutput::SetColors(uint32_t foregroundColor, uint32_t backgroundColor)
{
    (void)foregroundColor;
    (void)backgroundColor;
}



void IConsoleTextOutput::Clear()
{
}



void IConsoleTextOutput::EnableCursor(bool visible)
{
    (void)visible;
}



void IConsoleTextOutput::SetCursorPosition(int x, int y)
{
    (void)x;
    (void)y;
}



void IConsoleTextOutput::Rainbow()
{
    // VGA color indices for Rainbow:
    // 4 - red
    // 6 - brown
    // 14 - yellow
    // 3 - cyan
    // 9 - light blue
    // 5 - magenta
    // 7 - light gray
    SetColors(COLOR_RAINBOW_RED,    COLOR_BLACK); PutChar('R');
    SetColors(COLOR_RAINBOW_ORANGE, COLOR_BLACK); PutChar('a');
    SetColors(COLOR_RAINBOW_YELLOW, COLOR_BLACK); PutChar('i');
    SetColors(COLOR_RAINBOW_GREEN,  COLOR_BLACK); PutChar('n');
    SetColors(COLOR_RAINBOW_BLUE,   COLOR_BLACK); PutChar('b');
    SetColors(COLOR_RAINBOW_INDIGO, COLOR_BLACK); PutChar('o');
    SetColors(COLOR_RAINBOW_VIOLET, COLOR_BLACK); PutChar('w');
    SetColors(COLOR_VGA_LIGHT_GRAY, COLOR_BLACK); PutChar(' ');
}
