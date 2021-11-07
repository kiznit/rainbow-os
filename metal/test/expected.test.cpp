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

#include <metal/expected.hpp>
#include <unittest.hpp>

namespace Catch
{
    template <typename T>
    struct StringMaker<mtl::unexpected<T>>
    {
        static std::string convert(const mtl::unexpected<T>& u)
        {
            return StringMaker<T>::convert(u.value());
        }
    };

    template <typename T, typename E>
    struct StringMaker<mtl::expected<T, E>>
    {
        static std::string convert(const mtl::expected<T, E>& e)
        {
            if (e.has_value())
            {
                return StringMaker<T>::convert(e.value());
            }
            else
            {
                return StringMaker<E>::convert(e.error());
            }
        }
    };
} // namespace Catch

TEST_CASE("expected<> constructors")
{
    SECTION("default")
    {
        const mtl::expected<int, int> e;
        REQUIRE(e.has_value());
        REQUIRE(*e == 0);
        REQUIRE(e.value() == 0);
    }

    SECTION("from value")
    {
        const mtl::expected<int, int> e{3};
        REQUIRE(e.has_value());
        REQUIRE(*e == 3);
    }

    SECTION("from error")
    {
        const mtl::expected<int, int> e{mtl::unexpected(7)};
        REQUIRE(!e.has_value());
        REQUIRE(e.error() == 7);
    }

    SECTION("copy - value")
    {
        const mtl::expected<int, int> e{3};
        mtl::expected<int, int> c(e);
        REQUIRE(c.has_value());
        REQUIRE(*c == 3);
    }

    SECTION("copy - error")
    {
        const mtl::expected<int, int> e{mtl::unexpected(7)};
        mtl::expected<int, int> c(e);
        REQUIRE(!c.has_value());
        REQUIRE(c.error() == 7);
    }

    SECTION("move - value")
    {
        mtl::expected<int, int> e{3};
        const mtl::expected<int, int> c{std::move(e)};
        REQUIRE(c.has_value());
        REQUIRE(*c == 3);
    }

    SECTION("move - error")
    {
        mtl::expected<int, int> e{mtl::unexpected(7)};
        const mtl::expected<int, int> c{std::move(e)};
        REQUIRE(!c.has_value());
        REQUIRE(c.error() == 7);
    }
}

TEST_CASE("expected<> comparisons")
{
    const mtl::expected<int, int> v1{3};
    const mtl::expected<int, int> v2{3};
    const mtl::expected<int, int> v3{4};

    const mtl::unexpected<int> u1{3};
    const mtl::unexpected<int> u2{3};
    const mtl::unexpected<int> u3{4};

    const mtl::expected<int, int> e1{u1};
    const mtl::expected<int, int> e2{u2};
    const mtl::expected<int, int> e3{u3};

    SECTION("==")
    {
        REQUIRE(v1 == v2);
        REQUIRE(!(v1 == v3));

        REQUIRE(v1 == 3);
        REQUIRE(3 == v1);

        REQUIRE(!(v1 == 5));
        REQUIRE(!(5 == v1));

        REQUIRE(e1 == e2);
        REQUIRE(!(e1 == e3));

        REQUIRE(e1 == u1);
        REQUIRE(u1 == e1);
        REQUIRE(!(e1 == u3));
        REQUIRE(!(u3 == e1));
    }

    SECTION("!=")
    {
        REQUIRE(v1 != v3);
        REQUIRE(!(v1 != v2));

        REQUIRE(v1 != 4);
        REQUIRE(4 != v1);

        REQUIRE(!(v1 != 3));
        REQUIRE(!(3 != v1));

        REQUIRE(e1 != e3);
        REQUIRE(!(e1 != e2));

        REQUIRE(e1 != u3);
        REQUIRE(u3 != e1);
        REQUIRE(!(e1 != u2));
        REQUIRE(!(u2 != e1));
    }
}

TEST_CASE("expected<> assignments")
{
    SECTION("value")
    {
        mtl::expected<int, int> a{1};
        mtl::expected<int, int> b{mtl::unexpected{2}};

        a = 4;
        b = 5;

        REQUIRE(a.has_value());
        REQUIRE(a.value() == 4);
        REQUIRE(b.has_value());
        REQUIRE(b.value() == 5);
    }

    SECTION("expected")
    {
        const mtl::expected<int, int> a{1};
        mtl::expected<int, int> b{2};
        mtl::expected<int, int> c{mtl::unexpected{3}};

        b = a;
        c = a;

        REQUIRE(b.has_value());
        REQUIRE(b.value() == 1);
        REQUIRE(c.has_value());
        REQUIRE(c.value() == 1);
    }

    SECTION("unexpected")
    {
        const mtl::expected<int, int> a{mtl::unexpected{1}};
        mtl::expected<int, int> b{2};
        mtl::expected<int, int> c{mtl::unexpected{3}};

        b = a;
        c = a;

        REQUIRE(!b.has_value());
        REQUIRE(b.error() == 1);
        REQUIRE(!c.has_value());
        REQUIRE(c.error() == 1);
    }
}

TEST_CASE("expected<> swap")
{
    SECTION("unexpected")
    {
        mtl::unexpected<int> u1{1};
        mtl::unexpected<int> u2{2};

        mtl::swap(u1, u2);

        REQUIRE(u1.value() == 2);
        REQUIRE(u2.value() == 1);
    }

    SECTION("value / value")
    {
        mtl::expected<int, int> e1{1};
        mtl::expected<int, int> e2{2};

        mtl::swap(e1, e2);

        REQUIRE(e1.has_value());
        REQUIRE(e2.has_value());
        REQUIRE(e1.value() == 2);
        REQUIRE(e2.value() == 1);
    }

    SECTION("value / error")
    {
        mtl::expected<int, int> e1{1};
        mtl::expected<int, int> e2{mtl::unexpected{2}};

        mtl::swap(e1, e2);

        REQUIRE(!e1.has_value());
        REQUIRE(e2.has_value());
        REQUIRE(e1.error() == 2);
        REQUIRE(e2.value() == 1);
    }

    SECTION("error / value")
    {
        mtl::expected<int, int> e1{mtl::unexpected{1}};
        mtl::expected<int, int> e2{2};

        mtl::swap(e1, e2);

        REQUIRE(e1.has_value());
        REQUIRE(!e2.has_value());
        REQUIRE(e1.value() == 2);
        REQUIRE(e2.error() == 1);
    }

    SECTION("error / error")
    {
        mtl::expected<int, int> e1{mtl::unexpected{1}};
        mtl::expected<int, int> e2{mtl::unexpected{2}};

        mtl::swap(e1, e2);

        REQUIRE(!e1.has_value());
        REQUIRE(!e2.has_value());
        REQUIRE(e1.error() == 2);
        REQUIRE(e2.error() == 1);
    }
}