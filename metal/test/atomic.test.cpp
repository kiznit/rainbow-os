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

#include <c++/atomic>
#include <unittest.hpp>

template <typename T>
using atomic = std_test::atomic<T>;

TEST_CASE("Constructor", "[atomic]")
{
    SECTION("Default")
    {
        atomic<int> x;
        REQUIRE(x == 0);
    }

    SECTION("With value")
    {
        atomic<int> x{34};
        REQUIRE(x == 34);
    }
}

TEST_CASE("Assignment", "[atomic]")
{
    atomic<int> x;
    REQUIRE(x == 0);

    REQUIRE((x = 6) == 6);
    REQUIRE(x == 6);
}

TEST_CASE("Load and store", "[atomic]")
{
    atomic<int> x;
    REQUIRE(x == 0);
    REQUIRE(x.load() == 0);

    x.store(12);
    REQUIRE(x == 12);

    REQUIRE(x.load() == 12);
}

TEST_CASE("Exchange", "[atomic]")
{
    atomic<int> x{20};
    REQUIRE(x == 20);

    x.exchange(7);
    REQUIRE(x == 7);
}

TEST_CASE("operator++", "[atomic]")
{
    atomic<int> x;
    REQUIRE(x == 0);

    REQUIRE(++x == 1);
    REQUIRE(x == 1);
    REQUIRE(x++ == 1);
    REQUIRE(x == 2);
}

TEST_CASE("operator--", "[atomic]")
{
    atomic<int> x{2};
    REQUIRE(x == 2);

    REQUIRE(--x == 1);
    REQUIRE(x == 1);
    REQUIRE(x-- == 1);
    REQUIRE(x == 0);
}

TEST_CASE("fetch_add", "[atomic]")
{
    atomic<int> x{10};
    REQUIRE(x == 10);

    REQUIRE(x.fetch_add(5) == 10);
    REQUIRE(x == 15);
}

TEST_CASE("fetch_sub", "[atomic]")
{
    atomic<int> x{10};
    REQUIRE(x == 10);

    REQUIRE(x.fetch_sub(3) == 10);
    REQUIRE(x == 7);
}
