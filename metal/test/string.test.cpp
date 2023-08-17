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
    CHECK(s.length() == 0);
    CHECK(s.capacity() == 23);
}

TEST_CASE("Contruct from pointer and length", "[string]")
{
    SECTION("small string under max capacity")
    {
        _STD::string s("abc", 3);
        CHECK_THAT(s.c_str(), Equals("abc"));
        CHECK(s.length() == 3);
        CHECK(s.capacity() == 23);
    }

    SECTION("small string at max capacity")
    {
        _STD::string s("abcdefghijklmnopqrstuvw");
        CHECK_THAT(s.c_str(), Equals("abcdefghijklmnopqrstuvw"));
        CHECK(s.length() == 23);
        CHECK(s.capacity() == 23);
    }

    SECTION("large string")
    {
        _STD::string s("abcdefghijklmnopqrstuvwx");
        CHECK_THAT(s.c_str(), Equals("abcdefghijklmnopqrstuvwx"));
        CHECK(s.length() == 24);
        CHECK(s.capacity() == 39);
    }
}
