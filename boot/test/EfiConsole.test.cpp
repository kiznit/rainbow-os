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

#include <memory>
#include <string_view>
#include <unittest.hpp>
#include "EfiConsole.hpp"

using namespace metal;
using namespace std::literals;
using namespace trompeloeil;


struct MockSimpleTextOutputProtocol
{
    MAKE_MOCK2(OutputString, efi::Status(efi::SimpleTextOutputProtocol*, const wchar_t*));
};


TEST_CASE("EfiConsole", "[efi]")
{
    static MockSimpleTextOutputProtocol mock;

    efi::SimpleTextOutputProtocol conOut =
    {
        .OutputString = [](efi::SimpleTextOutputProtocol* self, const wchar_t* string) EFIAPI -> efi::Status
        {
            return mock.OutputString(self, string);
        },
        .SetAttribute = [](efi::SimpleTextOutputProtocol* self, efi::TextAttribute attribute) EFIAPI -> efi::Status
        {
            (void)self;
            (void)attribute;
            return efi::Success;
        },
    };

    EfiConsole console(&conOut);

    SECTION("ASCII string")
    {
        LogRecord record{ true, LogSeverity::Info, u8"Hello world"sv };
        REQUIRE_CALL(mock, OutputString(eq(&conOut), eq(L"Info   "))).RETURN(efi::Success);
        REQUIRE_CALL(mock, OutputString(eq(&conOut), eq(L": "))).RETURN(efi::Success);
        REQUIRE_CALL(mock, OutputString(eq(&conOut), eq(L"Hello world\n\r"))).RETURN(efi::Success);
        console.Log(record);
    }

    SECTION("French string")
    {
        LogRecord record{ true, LogSeverity::Info, u8"Retour à l'école"sv };
        REQUIRE_CALL(mock, OutputString(eq(&conOut), eq(L"Info   "))).RETURN(efi::Success);
        REQUIRE_CALL(mock, OutputString(eq(&conOut), eq(L": "))).RETURN(efi::Success);
        REQUIRE_CALL(mock, OutputString(eq(&conOut), eq(L"Retour à l'école\n\r"))).RETURN(efi::Success);
        console.Log(record);
    }

    SECTION("4-bytes UTF-8")
    {
        LogRecord record{ true, LogSeverity::Info, u8"\U0001f64a"sv };
        REQUIRE_CALL(mock, OutputString(eq(&conOut), eq(L"Info   "))).RETURN(efi::Success);
        REQUIRE_CALL(mock, OutputString(eq(&conOut), eq(L": "))).RETURN(efi::Success);
        REQUIRE_CALL(mock, OutputString(eq(&conOut), eq(L"?\n\r"))).RETURN(efi::Success);
        console.Log(record);
    }
}
