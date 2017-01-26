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

#include <Uefi.h>



extern "C" EFI_STATUS EFIAPI efi_main(EFI_HANDLE hImage, EFI_SYSTEM_TABLE* systemTable)
{
    if (!hImage || !systemTable)
        return EFI_INVALID_PARAMETER;

/*
    Initialize(hImage, systemTable);

    g_bootInfo.version = RAINBOW_BOOT_VERSION;
    g_bootInfo.firmware = Firmware_EFI;

    // Welcome message
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* output = g_efiSystemTable->ConOut;
    if (output)
    {
        const int32_t backupAttributes = output->Mode->Attribute;

        output->SetAttribute(output, EFI_TEXT_ATTR(EFI_RED,         EFI_BLACK)); putchar('R');
        output->SetAttribute(output, EFI_TEXT_ATTR(EFI_LIGHTRED,    EFI_BLACK)); putchar('a');
        output->SetAttribute(output, EFI_TEXT_ATTR(EFI_YELLOW,      EFI_BLACK)); putchar('i');
        output->SetAttribute(output, EFI_TEXT_ATTR(EFI_LIGHTGREEN,  EFI_BLACK)); putchar('n');
        output->SetAttribute(output, EFI_TEXT_ATTR(EFI_LIGHTCYAN,   EFI_BLACK)); putchar('b');
        output->SetAttribute(output, EFI_TEXT_ATTR(EFI_LIGHTBLUE,   EFI_BLACK)); putchar('o');
        output->SetAttribute(output, EFI_TEXT_ATTR(EFI_LIGHTMAGENTA,EFI_BLACK)); putchar('w');

        output->SetAttribute(output, backupAttributes);

        printf(" EFI Bootloader (" STRINGIZE(EFI_ARCH) ")\n\n", (int)sizeof(void*)*8);
    }


    EFI_STATUS status = Boot();

    printf("Boot() returned with status %p\n", (void*)status);

    Shutdown();
*/
    return EFI_SUCCESS;
}
