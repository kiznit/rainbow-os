/*
    Copyright (c) 2025, Thierry Tremblay
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

#include "crt0.hpp"

efi::Status efi_main()
{
    const auto conout = g_efiSystemTable->conout;
    conout->ClearScreen(conout);
    conout->OutputString(conout, u"Hello, world!\r\n");

    const auto bootServices = g_efiSystemTable->bootServices;
    const auto conin = g_efiSystemTable->conin;

    efi::uintn_t event = 0;
    efi::InputKey key{};

    while (key.scanCode != 27)
    {
        bootServices->WaitForEvent(1, &conin->waitForKey, &event);
        conin->ReadKeyStroke(conin, &key);
        conin->Reset(conin, false);
        char16_t string[3] = {key.unicodeChar, 0, 0};
        if (key.unicodeChar == '\r')
            string[1] = '\n';
        conout->OutputString(conout, string);
    }

    return efi::Status::Success;
}
