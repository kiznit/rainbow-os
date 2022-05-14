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

#include "EfiConsole.hpp"
#include "mock.hpp"
#include <memory>
#include <metal/unicode.hpp>
#include <string_view>
#include <unittest.hpp>

using namespace mtl;
using namespace std::literals;
using namespace trompeloeil;

TEST_CASE("EfiConsole", "[efi]")
{
    MockSimpleTextOutputProtocol output;
    EfiConsole console(&output);

    ALLOW_CALL(output.mocks, SetAttribute(_, _)).RETURN(efi::Status::Success);

    SECTION("ASCII string")
    {
        LogRecord record{true, LogSeverity::Info, u8"Hello world"sv};
        REQUIRE_CALL(output.mocks, OutputString(eq(&output), eq(u"Info   "))).RETURN(efi::Status::Success);
        REQUIRE_CALL(output.mocks, OutputString(eq(&output), eq(u": "))).RETURN(efi::Status::Success);
        REQUIRE_CALL(output.mocks, OutputString(eq(&output), eq(u"Hello world"))).RETURN(efi::Status::Success);
        REQUIRE_CALL(output.mocks, OutputString(eq(&output), eq(u"\n\r"))).RETURN(efi::Status::Success);
        console.Log(record);
    }

    SECTION("French string")
    {
        LogRecord record{true, LogSeverity::Info, u8"Retour à l'école"sv};
        REQUIRE_CALL(output.mocks, OutputString(eq(&output), eq(u"Info   "))).RETURN(efi::Status::Success);
        REQUIRE_CALL(output.mocks, OutputString(eq(&output), eq(u": "))).RETURN(efi::Status::Success);
        REQUIRE_CALL(output.mocks, OutputString(eq(&output), eq(u"Retour à l'école"))).RETURN(efi::Status::Success);
        REQUIRE_CALL(output.mocks, OutputString(eq(&output), eq(u"\n\r"))).RETURN(efi::Status::Success);
        console.Log(record);
    }

    SECTION("4-bytes UTF-8")
    {
        LogRecord record{true, LogSeverity::Info, u8"\U0001f64a"sv};
        REQUIRE_CALL(output.mocks, OutputString(eq(&output), eq(u"Info   "))).RETURN(efi::Status::Success);
        REQUIRE_CALL(output.mocks, OutputString(eq(&output), eq(u": "))).RETURN(efi::Status::Success);
        REQUIRE_CALL(output.mocks, OutputString(eq(&output), eq(u"\uFFFD"))).RETURN(efi::Status::Success);
        REQUIRE_CALL(output.mocks, OutputString(eq(&output), eq(u"\n\r"))).RETURN(efi::Status::Success);
        console.Log(record);
    }
}
