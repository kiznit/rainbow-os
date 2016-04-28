
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

#include "eficonsole.hpp"



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

    //todo: move this to boot code!
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



void EfiTextOutput::SetColors(Color foregroundColor, Color backgroundColor)
{
    m_output->SetAttribute(m_output, EFI_TEXT_ATTR(foregroundColor, (backgroundColor & 7));
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
