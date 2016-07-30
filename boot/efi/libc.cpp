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

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <Uefi.h>

#include "../common/boot.hpp"



extern EFI_HANDLE              g_efiImage;
extern EFI_SYSTEM_TABLE*       g_efiSystemTable;
extern EFI_BOOT_SERVICES*      g_efiBootServices;
extern EFI_RUNTIME_SERVICES*   g_efiRuntimeServices;



extern "C" int _libc_print(const char* string, size_t length)
{
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* output = g_efiSystemTable->ConOut;

    if (!output)
        return EOF;

    CHAR16 buffer[200];
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
            output->OutputString(output, buffer);
            count = 0;
        }
    }

    if (count > 0)
    {
        buffer[count] = '\0';
        output->OutputString(output, buffer);
    }

    return length;
}



extern "C" int getchar()
{
    EFI_SIMPLE_TEXT_INPUT_PROTOCOL* input = g_efiSystemTable->ConIn;

    if (!input || !g_efiBootServices)
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



extern "C" void* malloc(size_t size)
{
    if (g_efiBootServices)
    {
        void* memory;
        EFI_STATUS status = g_efiBootServices->AllocatePool(EfiLoaderData, size, &memory);
        if (!EFI_ERROR(status))
            return memory;
    }

    assert(0 && "Out of memory");
    return NULL;
}



extern "C" void free(void* p)
{
    if (p && g_efiBootServices)
        g_efiBootServices->FreePool(p);
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
