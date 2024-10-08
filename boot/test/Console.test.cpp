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
#include "mock.hpp"
#include <metal/string_view.hpp>
#include <metal/unicode.hpp>
#include <unittest.hpp>

using namespace mtl;
using namespace mtl::literals;
using namespace trompeloeil;

TEST_CASE("Console", "[efi]")
{
    MockSimpleTextOutputProtocol conout;
    efi::SystemTable systemTable;
    systemTable.conout = &conout;
    Console console(&systemTable);

    ALLOW_CALL(conout.mocks, SetAttribute(_, _)).RETURN(efi::Status::Success);

    SECTION("ASCII string")
    {
        LogRecord record{true, LogSeverity::Info, u8"Hello world"s};
        REQUIRE_CALL(conout.mocks, OutputString(eq(&conout), eq(u"Info   "))).RETURN(efi::Status::Success);
        REQUIRE_CALL(conout.mocks, OutputString(eq(&conout), eq(u": "))).RETURN(efi::Status::Success);
        REQUIRE_CALL(conout.mocks, OutputString(eq(&conout), eq(u"Hello world"))).RETURN(efi::Status::Success);
        REQUIRE_CALL(conout.mocks, OutputString(eq(&conout), eq(u"\n\r"))).RETURN(efi::Status::Success);
        console.Log(record);
    }

    SECTION("French string")
    {
        LogRecord record{true, LogSeverity::Info, u8"Retour à l'école"s};
        REQUIRE_CALL(conout.mocks, OutputString(eq(&conout), eq(u"Info   "))).RETURN(efi::Status::Success);
        REQUIRE_CALL(conout.mocks, OutputString(eq(&conout), eq(u": "))).RETURN(efi::Status::Success);
        REQUIRE_CALL(conout.mocks, OutputString(eq(&conout), eq(u"Retour à l'école"))).RETURN(efi::Status::Success);
        REQUIRE_CALL(conout.mocks, OutputString(eq(&conout), eq(u"\n\r"))).RETURN(efi::Status::Success);
        console.Log(record);
    }

    SECTION("4-bytes UTF-8")
    {
        LogRecord record{true, LogSeverity::Info, u8"\U0001f64a"s};
        REQUIRE_CALL(conout.mocks, OutputString(eq(&conout), eq(u"Info   "))).RETURN(efi::Status::Success);
        REQUIRE_CALL(conout.mocks, OutputString(eq(&conout), eq(u": "))).RETURN(efi::Status::Success);
        REQUIRE_CALL(conout.mocks, OutputString(eq(&conout), eq(u"\uFFFD"))).RETURN(efi::Status::Success);
        REQUIRE_CALL(conout.mocks, OutputString(eq(&conout), eq(u"\n\r"))).RETURN(efi::Status::Success);
        console.Log(record);
    }
}
