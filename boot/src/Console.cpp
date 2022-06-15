/*
    Copyright (c) 2022, Thierry Tremblay
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

static constexpr efi::TextAttribute s_severityColours[6] = {
    efi::TextAttribute::LightGray,    // Trace
    efi::TextAttribute::LightCyan,    // Debug
    efi::TextAttribute::LightGreen,   // Info
    efi::TextAttribute::Yellow,       // Warning
    efi::TextAttribute::LightRed,     // Error
    efi::TextAttribute::LightMagenta, // Fatal
};

static constexpr const char16_t* s_severityText[6] = {u"Trace  ", u"Debug  ", u"Info   ", u"Warning", u"Error  ", u"Fatal  "};

Console::Console(efi::SimpleTextOutputProtocol* conout) : m_conout(conout)
{}

void Console::Log(const mtl::LogRecord& record)
{
    auto conout = m_conout;

    conout->SetAttribute(conout, s_severityColours[record.severity]);
    conout->OutputString(conout, s_severityText[record.severity]);

    conout->SetAttribute(conout, efi::TextAttribute::LightGray);
    conout->OutputString(conout, u": ");

    // Convert to UCS-2 as required by UEFI.
    auto message = mtl::to_u16string(record.message, mtl::Ucs2);
    conout->OutputString(conout, message.c_str());
    conout->OutputString(conout, u"\n\r\0");
}