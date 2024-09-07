/*
    Copyright (c) 2024, Thierry Tremblay
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

#include "Console.hpp"
#include <metal/unicode.hpp>

static constexpr efi::TextAttribute kSeverityColours[6] = {
    efi::TextAttribute::LightGray,    // Trace
    efi::TextAttribute::LightCyan,    // Debug
    efi::TextAttribute::LightGreen,   // Info
    efi::TextAttribute::Yellow,       // Warning
    efi::TextAttribute::LightRed,     // Error
    efi::TextAttribute::LightMagenta, // Fatal
};

static constexpr const char16_t* s_severityText[6] = {u"Trace  ", u"Debug  ", u"Info   ", u"Warning", u"Error  ", u"Fatal  "};

Console::Console(efi::SystemTable* systemTable) : m_systemTable(systemTable)
{
}

mtl::expected<char16_t, efi::Status> Console::GetChar()
{
    auto conin = m_systemTable->conin;

    for (;;)
    {
        efi::uintn_t index;
        auto status = m_systemTable->bootServices->WaitForEvent(1, &conin->waitForKey, &index);
        if (efi::Error(status))
            return mtl::unexpected(status);

        efi::InputKey key;
        status = conin->ReadKeyStroke(conin, &key);
        if (efi::Error(status))
        {
            if (status == efi::Status::NotReady)
                continue;

            return mtl::unexpected(status);
        }

        return key.unicodeChar;
    }
}

void Console::Log(const mtl::LogRecord& record)
{
    auto conout = m_systemTable->conout;

    conout->SetAttribute(conout, kSeverityColours[(int)record.severity]);
    conout->OutputString(conout, s_severityText[(int)record.severity]);

    conout->SetAttribute(conout, efi::TextAttribute::LightGray);
    conout->OutputString(conout, u": ");

    // Convert to UCS-2 as required by UEFI.
    auto message = mtl::ToU16String(record.message, mtl::Ucs2);
    conout->OutputString(conout, message.c_str());
    conout->OutputString(conout, u"\n\r\0");
}

void Console::Write(const char16_t* string)
{
    auto conout = m_systemTable->conout;
    conout->OutputString(conout, string);
}
