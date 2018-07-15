/*
    Copyright (c) 2017, Thierry Tremblay
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
#include <string.h>

#include <Uefi.h>

#include "console.hpp"


extern IConsole* g_console;

extern EFI_HANDLE              g_efiImage;
extern EFI_SYSTEM_TABLE*       g_efiSystemTable;
extern EFI_BOOT_SERVICES*      g_efiBootServices;
extern EFI_RUNTIME_SERVICES*   g_efiRuntimeServices;



extern "C" int _libc_print(const char* string)
{
    return g_console->Print(string);
}



extern "C" int getchar()
{
    if (!g_efiSystemTable || !g_efiBootServices)
        return EOF;

    EFI_SIMPLE_TEXT_INPUT_PROTOCOL* input = g_efiSystemTable->ConIn;
    if (!input)
        return EOF;

    for (;;)
    {
        EFI_STATUS status;

        size_t index;
        status = g_efiBootServices->WaitForEvent(1, &input->WaitForKey, &index);
        if (EFI_ERROR(status))
        {
            return EOF;
        }

        EFI_INPUT_KEY key;
        status = input->ReadKeyStroke(input, &key);
        if (EFI_ERROR(status))
        {
            if (status == EFI_NOT_READY)
                continue;

            return EOF;
        }

        return key.UnicodeChar;
    }
}



extern "C" void abort()
{
    getchar();

    if (g_efiRuntimeServices)
    {
        const char* error = "abort()";
        g_efiRuntimeServices->ResetSystem(EfiResetWarm, EFI_ABORTED, strlen(error), (void*)error);
    }

    for (;;)
    {
        asm("cli; hlt");
    }
}
