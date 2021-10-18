/*
    Copyright (c) 2021, Thierry Tremblay
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

#include "boot.hpp"
#include "EfiConsole.hpp"

efi::Handle             g_efiImage;
efi::SystemTable*       g_efiSystemTable;
efi::BootServices*      g_efiBootServices;
efi::RuntimeServices*   g_efiRuntimeServices;

static void PrintBanner(efi::SimpleTextOutputProtocol* console)
{
    console->SetAttribute(console, efi::BackgroundBlack);
    console->ClearScreen(console);

    console->SetAttribute(console, efi::Red);
    console->OutputString(console, L"R");
    console->SetAttribute(console, efi::LightRed);
    console->OutputString(console, L"a");
    console->SetAttribute(console, efi::Yellow);
    console->OutputString(console, L"i");
    console->SetAttribute(console, efi::LightGreen);
    console->OutputString(console, L"n");
    console->SetAttribute(console, efi::LightCyan);
    console->OutputString(console, L"b");
    console->SetAttribute(console, efi::LightBlue);
    console->OutputString(console, L"o");
    console->SetAttribute(console, efi::LightMagenta);
    console->OutputString(console, L"w");
    console->SetAttribute(console, efi::LightGray);

    console->OutputString(console, L" UEFI bootloader\n\r\n\r");
}


// Cannot use "main()" as the function name as this causes problems with mingw
efi::Status efi_main()
{
    const auto conOut = g_efiSystemTable->conOut;
    PrintBanner(conOut);

    EfiConsole console(conOut);
    metal::g_log.AddLogger(&console);

    METAL_LOG(Info) << u8"UEFI firmware vendor: " << g_efiSystemTable->firmwareVendor;
    METAL_LOG(Info) << u8"UEFI firmware revision: " << (g_efiSystemTable->firmwareRevision >> 16) << u8'.' << (g_efiSystemTable->firmwareRevision & 0xFFFF);

    for (;;);

    return efi::Success;
}
