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

#include <c++/memory>
#include <unittest.hpp>

template <typename T>
using shared_ptr = std_test::shared_ptr<T>;

template <typename T>
using weak_ptr = std_test::weak_ptr<T>;

template <class T, class... Args>
shared_ptr<T> make_shared(Args&&... args)
{
    return std_test::make_shared<T>(std::forward<Args>(args)...);
}

struct Base : public std_test::enable_shared_from_this<Base>
{
};

struct Derived : Base
{
};

TEST_CASE("shared_ptr - Constructor", "[shared_ptr]")
{
    SECTION("Default")
    {
        shared_ptr<int> x;
        REQUIRE(!x);
        REQUIRE(x == nullptr);
        REQUIRE(x.use_count() == 0);
    }

    SECTION("nullptr_t")
    {
        shared_ptr<int> x{nullptr};
        REQUIRE(!x);
        REQUIRE(x == nullptr);
        REQUIRE(x.use_count() == 0);
    }

    SECTION("with pointer")
    {
        shared_ptr<int> x(new int{123});
        REQUIRE(x);
        REQUIRE(*x == 123);
        REQUIRE(x.use_count() == 1);
    }

    SECTION("copy")
    {
        shared_ptr<int> x;
        shared_ptr<int> y(x);
        REQUIRE(y == nullptr);
        REQUIRE(y.use_count() == 0);
    }

    SECTION("move")
    {
        auto x = make_shared<int>(21);
        shared_ptr<int> y(std::move(x));

        REQUIRE(!x);
        REQUIRE(x.use_count() == 0);

        REQUIRE(*y == 21);
        REQUIRE(y.use_count() == 1);
    }

    SECTION("with conversion")
    {
        auto x = make_shared<Derived>();
        shared_ptr<Base> y(x);
        REQUIRE(x == y);
        REQUIRE(y.use_count() == 2);
    }
}

TEST_CASE("shared_ptr - Assignment", "[shared_ptr]")
{
    SECTION("simple")
    {
        auto x = make_shared<int>(10);
        shared_ptr<int> y;
        y = x;
        REQUIRE(x == y);
        REQUIRE(y.use_count() == 2);
    }

    SECTION("with conversion")
    {
        auto x = make_shared<Derived>();
        shared_ptr<Base> y;
        y = x;
        REQUIRE(x == y);
        REQUIRE(y.use_count() == 2);
    }
}

TEST_CASE("shared_ptr - reset", "[shared_ptr]")
{
    SECTION("simple")
    {
        auto x = make_shared<int>(10);
        REQUIRE(*x == 10);

        x.reset();
        REQUIRE(x == nullptr);
    }

    SECTION("with conversion")
    {
        auto x = make_shared<Base>();
        REQUIRE(x != nullptr);
        REQUIRE(x.use_count() == 1);

        auto y = new Derived();
        x.reset(y);
        REQUIRE(x.get() == y);
        REQUIRE(x.use_count() == 1);
    }
}

TEST_CASE("weak_ptr - default constructor", "[shared_ptr]")
{
    weak_ptr<int> x;
    REQUIRE(x.expired());

    auto s = x.lock();
    REQUIRE(!s);
}

TEST_CASE("weak_ptr - copy constructor", "[shared_ptr]")
{
    auto s = make_shared<int>(123);
    weak_ptr<int> x(s);
    weak_ptr<int> y(x);

    REQUIRE(!x.expired());
    REQUIRE(!y.expired());

    REQUIRE(x.lock() == s);
    REQUIRE(y.lock() == s);
}

TEST_CASE("weak_ptr - move constructor", "[shared_ptr]")
{
    auto s = make_shared<int>(123);
    weak_ptr<int> x(s);
    weak_ptr<int> y(std::move(x));

    REQUIRE(x.expired());
    REQUIRE(!y.expired());

    REQUIRE(x.lock() == nullptr);
    REQUIRE(y.lock() == s);
}

TEST_CASE("weak_ptr - basic usage", "[shared_ptr]")
{
    auto s = make_shared<int>(123);
    weak_ptr<int> w(s);

    {
        auto x = w.lock();
        REQUIRE(x);
        REQUIRE(*x == 123);
    }

    {
        s.reset();
        auto y = w.lock();
        REQUIRE(!y);
    }
}

TEST_CASE("weak_ptr - reset", "[shared_ptr]")
{
    auto s = make_shared<int>(123);
    weak_ptr<int> w(s);

    REQUIRE(!w.expired());

    w.reset();
    REQUIRE(w.expired());

    auto x = w.lock();
    REQUIRE(!x);
}

TEST_CASE("shared_from_this() - 1", "[shared_ptr]")
{
    auto x = make_shared<Base>();
    REQUIRE(x.use_count() == 1);
    auto y = x->shared_from_this();
    REQUIRE(x.use_count() == 2);
    REQUIRE(x == y);

    x.reset();
    REQUIRE(y);
    REQUIRE(y.use_count() == 1);
}

TEST_CASE("shared_from_this() - 2", "[shared_ptr]")
{
    auto v = new Base();
    _STD::shared_ptr<Base> x(v);
    REQUIRE(x.get() == v);
    REQUIRE(x.use_count() == 1);

    auto y = x->shared_from_this();
    REQUIRE(x == y);
    REQUIRE(x.use_count() == 2);
}

TEST_CASE("weak_from_this() - 1", "[shared_ptr]")
{
    auto x = make_shared<Base>();
    auto y = x->weak_from_this();
    REQUIRE(!y.expired());
    REQUIRE(x == y.lock());

    x.reset();
    REQUIRE(y.expired());
}

TEST_CASE("weak_from_this() - 2", "[shared_ptr]")
{
    auto v = new Base();
    _STD::shared_ptr<Base> x(v);
    REQUIRE(x.get() == v);
    REQUIRE(x.use_count() == 1);

    auto y = x->weak_from_this();
    REQUIRE(!y.expired());
    REQUIRE(x == y.lock());
    REQUIRE(x.use_count() == 1);
}
