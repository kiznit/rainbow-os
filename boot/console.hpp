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

#include "colors.hpp"



// Console Text Input Inteface

class IConsoleTextInput
{
public:

    // Blocking call to read a key press, works just like libc's getchar()
    virtual int GetChar();
};



// Console Text Output Inteface

class IConsoleTextOutput
{
public:

    // Output a character on the screen, works just like libc's putchar()
    virtual int PutChar(int c);

    // Ouput 'length' characters from 'string'
    virtual int Print(const char* string, size_t length);

    // Change text color attributes
    virtual void SetColors(uint32_t foregroundColor, uint32_t backgroundColor);

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
