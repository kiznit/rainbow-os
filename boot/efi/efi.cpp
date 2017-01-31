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

#include <stdio.h>
#include <stdlib.h>
#include <Uefi.h>

#include "boot.hpp"


static EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* g_console;



extern "C" int _libc_print(const char* string)
{
    if (!g_console)
        return EOF;

    size_t length = 0;

    CHAR16 buffer[200];
    size_t count = 0;

    for (const char* p = string; *p; ++p, ++length)
    {
        const unsigned char c = *p;

        if (c == '\n')
            buffer[count++] = '\r';

        buffer[count++] = c;

        if (count >= ARRAY_LENGTH(buffer) - 3)
        {
            buffer[count] = '\0';
            g_console->OutputString(g_console, buffer);
            count = 0;
        }
    }

    if (count > 0)
    {
        buffer[count] = '\0';
        g_console->OutputString(g_console, buffer);
    }

    return length;
}



static void InitConsole(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* console)
{
    // Mode 0 is always 80x25 text mode and is always supported
    // Mode 1 is always 80x50 text mode and isn't always supported
    // Modes 2+ are differents on every device
    size_t mode = 0;
    size_t width = 80;
    size_t height = 25;

    for (size_t m = 0; ; ++m)
    {
        size_t  w, h;
        EFI_STATUS status = console->QueryMode(console, m, &w, &h);
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

    console->SetMode(console, mode);

    // Some firmware won't clear the screen and/or reset the text colors on SetMode().
    // This is presumably more likely to happen when the selected mode is the current one.
    console->SetAttribute(console, EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLACK));
    console->ClearScreen(console);
    console->EnableCursor(console, FALSE);
    console->SetCursorPosition(console, 0, 0);

    g_console = console;
}



extern "C" EFI_STATUS EFIAPI efi_main(EFI_HANDLE hImage, EFI_SYSTEM_TABLE* systemTable)
{
    if (!hImage || !systemTable)
        return EFI_INVALID_PARAMETER;

    // Welcome message
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* console = systemTable->ConOut;

    if (console)
    {
        InitConsole(console);

        console->SetAttribute(console, EFI_TEXT_ATTR(EFI_RED,         EFI_BLACK)); console->OutputString(console, (CHAR16*)L"R");
        console->SetAttribute(console, EFI_TEXT_ATTR(EFI_LIGHTRED,    EFI_BLACK)); console->OutputString(console, (CHAR16*)L"a");
        console->SetAttribute(console, EFI_TEXT_ATTR(EFI_YELLOW,      EFI_BLACK)); console->OutputString(console, (CHAR16*)L"i");
        console->SetAttribute(console, EFI_TEXT_ATTR(EFI_LIGHTGREEN,  EFI_BLACK)); console->OutputString(console, (CHAR16*)L"n");
        console->SetAttribute(console, EFI_TEXT_ATTR(EFI_LIGHTCYAN,   EFI_BLACK)); console->OutputString(console, (CHAR16*)L"b");
        console->SetAttribute(console, EFI_TEXT_ATTR(EFI_LIGHTBLUE,   EFI_BLACK)); console->OutputString(console, (CHAR16*)L"o");
        console->SetAttribute(console, EFI_TEXT_ATTR(EFI_LIGHTMAGENTA,EFI_BLACK)); console->OutputString(console, (CHAR16*)L"w");
        console->SetAttribute(console, EFI_TEXT_ATTR(EFI_LIGHTGRAY,   EFI_BLACK));

        printf(" EFI Bootloader (" STRINGIZE(EFI_ARCH) ")\n\r\n\r");
    }


    for (;;);

    // printf("\nPress any key to exit");
    // getchar();
    // printf("\nExiting...");

    return EFI_SUCCESS;
}
