/*
    Copyright (c) 2023, Thierry Tremblay
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

#include <c++/string>
#include <unittest.hpp>

using Catch::Matchers::Equals;

TEST_CASE("Default constructor", "[string]")
{
    _STD::string s;
    CHECK_THAT(s.c_str(), Equals(""));
    CHECK(s == "");
    CHECK(s.length() == 0);
    CHECK(s.capacity() == 23);
}

TEST_CASE("Contruct from pointer and length", "[string]")
{
    SECTION("small string under max capacity")
    {
        _STD::string s("abc", 3);
        CHECK_THAT(s.c_str(), Equals("abc"));
        CHECK(s == "abc");
        CHECK(s != "def");
        CHECK(s.length() == 3);
        CHECK(s.capacity() == 23);
    }

    SECTION("small string at max capacity")
    {
        _STD::string s("abcdefghijklmnopqrstuvw");
        CHECK(s == "abcdefghijklmnopqrstuvw");
        CHECK(s.length() == 23);
        CHECK(s.capacity() == 23);
    }

    SECTION("large string")
    {
        _STD::string s("abcdefghijklmnopqrstuvwx");
        CHECK(s == "abcdefghijklmnopqrstuvwx");
        CHECK(s.length() == 24);
        CHECK(s.capacity() == 39);
    }
}

TEST_CASE("Move", "[string]")
{
    SECTION("small string, construction")
    {
        _STD::string a("abc");
        _STD::string b(std::move(a));
        CHECK(a == "abc");
        CHECK(b == "abc");
    }

    SECTION("large string, construction")
    {
        _STD::string a("abcdefghijklmnopqrstuvwx");
        _STD::string b(std::move(a));
        CHECK(a == "");
        CHECK(b == "abcdefghijklmnopqrstuvwx");
    }

    SECTION("small string, assignment")
    {
        _STD::string a("abc");
        _STD::string b;
        b = std::move(a);
        CHECK(a == "abc");
        CHECK(b == "abc");
    }

    SECTION("large string, assignment")
    {
        _STD::string a("abcdefghijklmnopqrstuvwx");
        _STD::string b;
        b = std::move(a);
        CHECK(a == "");
        CHECK(b == "abcdefghijklmnopqrstuvwx");
    }
}

TEST_CASE("u16string", "[string]")
{
    SECTION("Default constructor")
    {
        _STD::u16string s;
        CHECK(s == u"");
        CHECK(s.length() == 0);
        CHECK(s.capacity() == 11);
    }

    SECTION("small string at max capacity")
    {
        _STD::u16string s(u"abcdefghijk");
        CHECK(s == u"abcdefghijk");
        CHECK(s.length() == 11);
        CHECK(s.capacity() == 11);
    }

    SECTION("large string")
    {
        _STD::u16string s(u"abcdefghijkl");
        CHECK(s == u"abcdefghijkl");
        CHECK(s.length() == 12);
        CHECK(s.capacity() == 19);
    }
}

TEST_CASE("u32string", "[string]")
{
    SECTION("Default constructor")
    {
        _STD::u32string s;
        CHECK(s == U"");
        CHECK(s.length() == 0);
        CHECK(s.capacity() == 5);
    }

    SECTION("small string at max capacity")
    {
        _STD::u32string s(U"abcde");
        CHECK(s == U"abcde");
        CHECK(s.length() == 5);
        CHECK(s.capacity() == 5);
    }

    SECTION("large string")
    {
        _STD::u32string s(U"abcdef");
        CHECK(s == U"abcdef");
        CHECK(s.length() == 6);
        CHECK(s.capacity() == 9);
    }
}
