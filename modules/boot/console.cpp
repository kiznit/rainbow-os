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



void IConsoleTextOutput::SetColors(Color foregroundColor, Color backgroundColor)
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
    SetColors(Color_Red,        Color_Black); PutChar('R');
    SetColors(Color_Brown,      Color_Black); PutChar('a');
    SetColors(Color_Yellow,     Color_Black); PutChar('i');
    SetColors(Color_LightGreen, Color_Black); PutChar('n');
    SetColors(Color_Cyan,       Color_Black); PutChar('b');
    SetColors(Color_LightBlue,  Color_Black); PutChar('o');
    SetColors(Color_Magenta,    Color_Black); PutChar('w');
    SetColors(Color_LightGray,  Color_Black); PutChar(' ');
}
