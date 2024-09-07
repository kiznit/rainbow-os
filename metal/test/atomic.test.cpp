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

#include <metal/atomic.hpp>
#include <unittest.hpp>

TEST_CASE("atomic - Constructor", "[atomic]")
{
    SECTION("Default")
    {
        mtl::atomic<int> x;
        REQUIRE(x == 0);
    }

    SECTION("With value")
    {
        mtl::atomic<int> x{34};
        REQUIRE(x == 34);
    }
}

TEST_CASE("atomic - Assignment", "[atomic]")
{
    mtl::atomic<int> x;
    REQUIRE(x == 0);

    REQUIRE((x = 6) == 6);
    REQUIRE(x == 6);
}

TEST_CASE("atomic - Load and store", "[atomic]")
{
    mtl::atomic<int> x;
    REQUIRE(x == 0);
    REQUIRE(x.load() == 0);

    x.store(12);
    REQUIRE(x == 12);

    REQUIRE(x.load() == 12);
}

TEST_CASE("atomic - exchange", "[atomic]")
{
    mtl::atomic<int> x{20};
    REQUIRE(x == 20);

    x.exchange(7);
    REQUIRE(x == 7);
}

TEST_CASE("atomic - compare_exchange_strong", "[atomic]")
{
    mtl::atomic<int> x{10};
    int expected = 10;
    REQUIRE(x.compare_exchange_strong(expected, 20));
    REQUIRE(x == 20);
    REQUIRE(expected == 10);

    mtl::atomic<int> y{5};
    expected = 2;
    REQUIRE(!y.compare_exchange_strong(expected, 3));
    REQUIRE(y == 5);
    REQUIRE(expected == 5);
}

TEST_CASE("atomic - compare_exchange_weak", "[atomic]")
{
    mtl::atomic<int> x{10};
    int expected = 10;
    REQUIRE(x.compare_exchange_weak(expected, 20));
    REQUIRE(x == 20);
    REQUIRE(expected == 10);

    mtl::atomic<int> y{5};
    expected = 2;
    REQUIRE(!y.compare_exchange_weak(expected, 3));
    REQUIRE(y == 5);
    REQUIRE(expected == 5);
}

TEST_CASE("atomic - operator++", "[atomic]")
{
    mtl::atomic<int> x;
    REQUIRE(x == 0);

    REQUIRE(++x == 1);
    REQUIRE(x == 1);
    REQUIRE(x++ == 1);
    REQUIRE(x == 2);
}

TEST_CASE("atomic - operator--", "[atomic]")
{
    mtl::atomic<int> x{2};
    REQUIRE(x == 2);

    REQUIRE(--x == 1);
    REQUIRE(x == 1);
    REQUIRE(x-- == 1);
    REQUIRE(x == 0);
}

TEST_CASE("atomic - fetch_add", "[atomic]")
{
    mtl::atomic<int> x{10};
    REQUIRE(x == 10);

    REQUIRE(x.fetch_add(5) == 10);
    REQUIRE(x == 15);
}

TEST_CASE("atomic - fetch_sub", "[atomic]")
{
    mtl::atomic<int> x{10};
    REQUIRE(x == 10);

    REQUIRE(x.fetch_sub(3) == 10);
    REQUIRE(x == 7);
}
