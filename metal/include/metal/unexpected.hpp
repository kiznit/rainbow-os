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

#pragma once

#include <initializer_list>
#include <type_traits>
#include <utility>

namespace mtl
{
    using std::in_place_t;
    using std::initializer_list;

    struct unexpect_t
    {
        explicit unexpect_t() = default;
    };

    inline constexpr unexpect_t unexpect{};

    template <class E>
    class unexpected
    {
    public:
        // Constructors
        constexpr unexpected(const unexpected&) = default;
        constexpr unexpected(unexpected&&) = default;

        template <class... Args>
        constexpr explicit unexpected(in_place_t, Args&&... args)
            requires(std::is_constructible_v<E, Args...>)
            : _value(std::forward<Args>(args)...)
        {
        }

        template <class U, class... Args>
        constexpr explicit unexpected(in_place_t, initializer_list<U> il, Args&&... args)
            requires(std::is_constructible_v<E, initializer_list<U>&, Args...>)
            : _value(il, std::forward<Args>(args)...)
        {
        }

        template <class Err = E>
        constexpr explicit unexpected(Err&& e)
            requires(!std::is_same_v<std::remove_cvref_t<Err>, unexpected> &&
                     !std::is_same_v<std::remove_cvref_t<Err>, in_place_t> && std::is_constructible_v<E, Err>)
            : _value(std::forward<Err>(e))
        {
        }

        // Assignment
        constexpr unexpected& operator=(const unexpected&) = default;
        constexpr unexpected& operator=(unexpected&&) = default;

        // Observers
        constexpr E& value() & noexcept { return _value; }
        constexpr const E& value() const& noexcept { return _value; }
        constexpr E&& value() && noexcept { return std::move(_value); }
        constexpr const E&& value() const&& noexcept { return std::move(_value); }

        // Swap
        constexpr void swap(unexpected& other) noexcept(std::is_nothrow_swappable_v<E>)
        {
            using std::swap;
            swap(_value, other._value);
        }

        friend void swap(unexpected<E>& x, unexpected<E>& y) noexcept(noexcept(x.swap(y)))
            requires(std::is_swappable_v<E>)
        {
            x.swap(y);
        }

        template <class E2>
        friend constexpr bool operator==(const unexpected& x, const unexpected<E2>& y)
        {
            return x.value() == y.value();
        }

#if defined(__GNUC__) && __GNUC__ < 10 && !defined(__clang__)
        template <class E2>
        friend constexpr bool operator!=(const unexpected& x, const unexpected<E2>& y)
        {
            return !(x == y);
        }
#endif

    private:
        E _value;
    };

    template <class E>
    unexpected(E) -> unexpected<E>;

} // namespace mtl