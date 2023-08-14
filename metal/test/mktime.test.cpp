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

#define VERIFY_TESTS 0

#if VERIFY_TESTS
#include <ctime>
#define _STD std
#else
#include "c++/ctime"
#define _STD std_test
#endif

#include <unittest.hpp>

#if VERIFY_TESTS
TEST_CASE("set UTC", "[time]")
{
    setenv("TZ", "", 1);
    tzset();
}
#endif

TEST_CASE("origin", "[mktime]")
{
    _STD::tm tm{};
    tm.tm_year = 1970 - 1900;
    tm.tm_mon = 0;
    tm.tm_mday = 1;
    tm.tm_isdst = -1;

    CHECK(0 == _STD::mktime(&tm));

    CHECK(tm.tm_yday == 0);
    CHECK(tm.tm_wday == 4);
    CHECK(tm.tm_isdst == 0);
}

TEST_CASE("after 1970", "[mktime]")
{
    _STD::tm tm{};
    tm.tm_year = 2023 - 1900;
    tm.tm_mon = 8 - 1;
    tm.tm_mday = 15;
    tm.tm_isdst = -1;

    REQUIRE(1692057600 == _STD::mktime(&tm));
    CHECK(tm.tm_yday == 226);
    CHECK(tm.tm_wday == 2);
    CHECK(tm.tm_isdst == 0);
}

TEST_CASE("before 1970", "[mktime]")
{
    _STD::tm tm{};
    tm.tm_year = 1800 - 1900;
    tm.tm_mon = 6;
    tm.tm_mday = 1;
    tm.tm_isdst = -1;

    REQUIRE(-5349024000 == _STD::mktime(&tm));
    _STD::mktime(&tm);
    CHECK(tm.tm_yday == 181);
    CHECK(tm.tm_wday == 2);
    CHECK(tm.tm_isdst == 0);
}

TEST_CASE("date with time", "[mktime]")
{
    _STD::tm tm{};
    tm.tm_year = 1972 - 1900;
    tm.tm_mon = 10 - 1;
    tm.tm_mday = 26;
    tm.tm_hour = 23;
    tm.tm_min = 02;
    tm.tm_sec = 27;
    tm.tm_isdst = -1;

    REQUIRE(88988547 == _STD::mktime(&tm));
    CHECK(tm.tm_yday == 299);
    CHECK(tm.tm_wday == 4);
    CHECK(tm.tm_isdst == 0);
}

TEST_CASE("leap second", "[mktime]")
{
    _STD::tm tm{};
    tm.tm_year = 2005 - 1900;
    tm.tm_mon = 12 - 1;
    tm.tm_mday = 31;
    tm.tm_hour = 23;
    tm.tm_min = 59;
    tm.tm_sec = 60;

    REQUIRE(1136073600 == _STD::mktime(&tm));
    CHECK(tm.tm_sec == 0);
    CHECK(tm.tm_min == 0);
    CHECK(tm.tm_hour == 0);
    CHECK(tm.tm_mday == 1);
    CHECK(tm.tm_mon == 0);
    CHECK(tm.tm_year == 2006 - 1900);
    CHECK(tm.tm_yday == 0);
    CHECK(tm.tm_wday == 0);
}

TEST_CASE("overflow", "[mktime]")
{
    _STD::tm tm{};
    tm.tm_year = 2020 - 1900;
    tm.tm_mon = 12 - 1;
    tm.tm_mday = 31;
    tm.tm_hour = 23;
    tm.tm_min = 59;
    tm.tm_sec = 60;

    REQUIRE(1609459200 == _STD::mktime(&tm));
    CHECK(tm.tm_sec == 0);
    CHECK(tm.tm_min == 0);
    CHECK(tm.tm_hour == 0);
    CHECK(tm.tm_mday == 1);
    CHECK(tm.tm_mon == 0);
    CHECK(tm.tm_year == 2021 - 1900);
    CHECK(tm.tm_yday == 0);
    CHECK(tm.tm_wday == 5);
}

TEST_CASE("underflow", "[mktime]")
{
    _STD::tm tm{};
    tm.tm_year = 2021 - 1900;
    tm.tm_mon = 1 - 1;
    tm.tm_mday = 1;
    tm.tm_hour = 0;
    tm.tm_min = 0;
    tm.tm_sec = -1;

    REQUIRE(1609459199 == _STD::mktime(&tm));
    CHECK(tm.tm_sec == 59);
    CHECK(tm.tm_min == 59);
    CHECK(tm.tm_hour == 23);
    CHECK(tm.tm_mday == 31);
    CHECK(tm.tm_mon == 12 - 1);
    CHECK(tm.tm_year == 2020 - 1900);
    CHECK(tm.tm_yday == 365);
    CHECK(tm.tm_wday == 4);
}
