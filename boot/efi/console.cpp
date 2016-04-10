
/*
    Copyright (c) 2016, Thierry Tremblay
    All rights reserved.

    Redistribution and use source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retathe above copyright notice, this
      list of conditions and the following disclaimer.

    * Redistributions binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer the documentation
      and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING ANY WAY OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "console.hpp"

#define ARRAY_LENGTH(array)     (sizeof(array) / sizeof((array)[0]))

// Colors taken from the Tianocore UEFI Development Kit.
// This also matches what VirtualBox uses.

#define COLOR_EFI_BLACK         0x000000
#define COLOR_EFI_BLUE          0x000098
#define COLOR_EFI_GREEN         0x009800
#define COLOR_EFI_CYAN          0x009898
#define COLOR_EFI_RED           0x980000
#define COLOR_EFI_MAGENTA       0x980098
#define COLOR_EFI_BROWN         0x989800
#define COLOR_EFI_LIGHT_GRAY    0x989898
#define COLOR_EFI_DARK_GRAY     0x303030
#define COLOR_EFI_LIGHT_BLUE    0x0000ff
#define COLOR_EFI_LIGHT_GREEN   0x00ff00
#define COLOR_EFI_LIGHT_CYAN    0x00c0ff
#define COLOR_EFI_LIGHT_RED     0xff0000
#define COLOR_EFI_LIGHT_MAGENTA 0xff00ff
#define COLOR_EFI_YELLOW        0xffff00
#define COLOR_EFI_WHITE         0xffffff


static const uint32_t s_efi_colors[16] =
{
    COLOR_EFI_BLACK,
    COLOR_EFI_BLUE,
    COLOR_EFI_GREEN,
    COLOR_EFI_CYAN,
    COLOR_EFI_RED,
    COLOR_EFI_MAGENTA,
    COLOR_EFI_BROWN,
    COLOR_EFI_LIGHT_GRAY,
    COLOR_EFI_DARK_GRAY,
    COLOR_EFI_LIGHT_BLUE,
    COLOR_EFI_LIGHT_GREEN,
    COLOR_EFI_LIGHT_CYAN,
    COLOR_EFI_LIGHT_RED,
    COLOR_EFI_LIGHT_MAGENTA,
    COLOR_EFI_YELLOW,
    COLOR_EFI_WHITE
};



void EfiTextOutput::Initialize(efi::SimpleTextOutputProtocol* output)
{
    m_output = output;

    // Mode 0 is always 80x25 text mode and is always supported
    // Mode 1 is always 80x50 text mode and isn't always supported
    // Modes 2+ are differents on every device
    size_t mode = 0;
    size_t width = 80;
    size_t height = 25;

    for (size_t m = 0; ; ++m)
    {
        size_t  w, h;
        efi::status_t status = output->QueryMode(output, m, &w, &h);
        if (EFI_ERROR(status))
        {
            // Mode 1 might return EFI_UNSUPPORTED, we still want to check modes 2+
            if (m > 1)
                break;
        }
        else
        {
            if (w * h > width * height)
            {
                mode = m;
                width = w;
                height = h;
            }
        }
    }

    output->SetMode(output, mode);

    EnableCursor(false);

    // Some firmware won't clear the screen and/or reset the text colors on SetMode().
    // This is presumably more likely to happen when the selected mode is the existing one.
    SetColors(COLOR_EFI_LIGHT_GRAY, COLOR_EFI_BLACK);
    Clear();
}



int EfiTextOutput::Print(const char* string, size_t length)
{
    wchar_t buffer[200];
    size_t count = 0;

    for (size_t i = 0; i != length; ++i)
    {
        const unsigned char c = string[i];

        if (c == '\n')
            buffer[count++] = '\r';

        buffer[count++] = c;

        if (count >= ARRAY_LENGTH(buffer) - 3)
        {
            buffer[count] = '\0';
            m_output->OutputString(m_output, buffer);
            count = 0;
        }
    }

    if (count > 0)
    {
        buffer[count] = '\0';
        m_output->OutputString(m_output, buffer);
    }

    return 1;
}



void EfiTextOutput::SetColors(uint32_t foregroundColor, uint32_t backgroundColor)
{
    int fg = FindNearestColor(foregroundColor, s_efi_colors, 16);
    int bg = FindNearestColor(backgroundColor, s_efi_colors, 8);

   m_output->SetAttribute(m_output, EFI_TEXT_ATTR(fg, bg));
}



void EfiTextOutput::Clear()
{
    m_output->ClearScreen(m_output);
}



void EfiTextOutput::EnableCursor(bool visible)
{
    m_output->EnableCursor(m_output, visible);
}



void EfiTextOutput::SetCursorPosition(int x, int y)
{
    m_output->SetCursorPosition(m_output, x, y);
}



void EfiTextOutput::Rainbow()
{
    // We don't use color matching here as we don't care about accuracy.
    // It is more important that it is readable and look nice.
    m_output->SetAttribute(m_output, EFI_TEXT_ATTR(EFI_RED,         EFI_BLACK)); PutChar('R');
    m_output->SetAttribute(m_output, EFI_TEXT_ATTR(EFI_LIGHTRED,    EFI_BLACK)); PutChar('a');
    m_output->SetAttribute(m_output, EFI_TEXT_ATTR(EFI_YELLOW,      EFI_BLACK)); PutChar('i');
    m_output->SetAttribute(m_output, EFI_TEXT_ATTR(EFI_LIGHTGREEN,  EFI_BLACK)); PutChar('n');
    m_output->SetAttribute(m_output, EFI_TEXT_ATTR(EFI_LIGHTCYAN,   EFI_BLACK)); PutChar('b');
    m_output->SetAttribute(m_output, EFI_TEXT_ATTR(EFI_LIGHTBLUE,   EFI_BLACK)); PutChar('o');
    m_output->SetAttribute(m_output, EFI_TEXT_ATTR(EFI_LIGHTMAGENTA,EFI_BLACK)); PutChar('w');
    m_output->SetAttribute(m_output, EFI_TEXT_ATTR(EFI_LIGHTGRAY,   EFI_BLACK)); PutChar(' ');
}
