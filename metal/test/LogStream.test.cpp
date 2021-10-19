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

#include <string_view>
#include <unittest.hpp>
#include <metal/log/stream.hpp>

using namespace metal;
using namespace std::literals;


TEST_CASE("operator<<", "[LogStream]")
{
    LogRecord record;
    LogStream stream(record);

    SECTION("char8_t")
    {
        stream << u8'a';
        stream.Flush();
        REQUIRE(record.message == u8"a");
    }

    SECTION("wchar_t")
    {
        stream << L'a';
        stream.Flush();
        REQUIRE(record.message == u8"a");
    }

    SECTION("u8string")
    {
        stream << u8"utf8";
        stream.Flush();
        REQUIRE(record.message == u8"utf8");
    }

    SECTION("wstring")
    {
        stream << L"wide";
        stream.Flush();
        REQUIRE(record.message == u8"wide");
    }

    SECTION("u8string_view")
    {
        stream << u8"utf8"sv;
        stream.Flush();
        REQUIRE(record.message == u8"utf8");
    }

    SECTION("wstring_view")
    {
        stream << L"wide"sv;
        stream.Flush();
        REQUIRE(record.message == u8"wide");
    }

    SECTION("long (positive)")
    {
        stream << 123l;
        stream.Flush();
        REQUIRE(record.message == u8"123");
    }

    SECTION("long (negative)")
    {
        stream << -512l;
        stream.Flush();
        REQUIRE(record.message == u8"-512");
    }

    SECTION("unsigned long")
    {
        stream << 0xFFFFFFFFu;
        stream.Flush();
        REQUIRE(record.message == u8"4294967295");
    }

    SECTION("long long (positive)")
    {
        stream << 9223372036854775807ll;
        stream.Flush();
        REQUIRE(record.message == u8"9223372036854775807");
    }

    SECTION("long long (negative)")
    {
        stream << (-9223372036854775807ll - 1);
        stream.Flush();
        REQUIRE(record.message == u8"-9223372036854775808");
    }

    SECTION("unsigned long long")
    {
        stream << 0xFFFFFFFFFFFFFFFFull;
        stream.Flush();
        REQUIRE(record.message == u8"18446744073709551615");
    }

    SECTION("pointer")
    {
        if (sizeof(void*) == 4)
        {
            stream << (void*)0xfe12;
            stream.Flush();
            REQUIRE(record.message == u8"0000fe12");

            stream << (struct Foo *)0xfe12;
            stream.Flush();
            REQUIRE(record.message == u8"0000fe12");
        }
        else if (sizeof(void*) == 8)
        {
            stream << (void*)0xf8001234abcd;
            stream.Flush();
            REQUIRE(record.message == u8"0000f8001234abcd");

            stream << (struct Foo *)0xf8001234abcd;
            stream.Flush();
            REQUIRE(record.message == u8"0000f8001234abcd");
        }
    }
}
