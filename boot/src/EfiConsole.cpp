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

#include "EfiConsole.hpp"
#include <metal/static_vector.hpp>
#include <metal/unicode.hpp>

static constexpr efi::TextAttribute s_colours[6] = {
    efi::LightGray,    // Trace
    efi::LightCyan,    // Debug
    efi::LightGreen,   // Info
    efi::Yellow,       // Warning
    efi::LightRed,     // Error
    efi::LightMagenta, // Fatal
};

// TODO: move this to a central place in metal as we don't want to repeat these strings over and
// over
static constexpr const wchar_t* s_descriptions[6] = {
    L"Trace  ", L"Debug  ", L"Info   ", L"Warning", L"Error  ", L"Fatal  ",
};

EfiConsole::EfiConsole(efi::SimpleTextOutputProtocol* console) : m_console(console)
{}

void EfiConsole::Log(const mtl::LogRecord& record)
{
    auto console = m_console;

    console->SetAttribute(console, s_colours[record.severity]);
    console->OutputString(console, s_descriptions[record.severity]);

    console->SetAttribute(console, efi::LightGray);
    console->OutputString(console, L": ");

    // Convert the utf-8 source to UCS-2 as required by UEFI.
    mtl::static_vector<wchar_t, 500> buffer;

    auto text = record.message.begin();
    auto end = record.message.end();
    while (text != end)
    {
        const auto codepoint = mtl::utf8_to_codepoint(text, end);
        if (mtl::is_valid_ucs2_codepoint(codepoint))
        {
            buffer.push_back(codepoint);
        }
        else
        {
            // We could push U+FFFD, but it won't print anything on the EFI console.
            buffer.push_back(L'?');
        }

        // We need to make sure there is space for at least 3 characters in
        // the buffer before we exit this loop (\n, \r and \0, see below).
        if (buffer.size() > buffer.capacity() - 3)
        {
            buffer.push_back(L'\0');
            console->OutputString(console, buffer.data());
            buffer.clear();
        }
    }

    // Append newline control characters
    buffer.push_back(L'\n');
    buffer.push_back(L'\r');
    buffer.push_back(L'\0');
    console->OutputString(console, buffer.data());
}
