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

#pragma once

#include <cassert>
#include <initializer_list>
#include <utility>

// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p0323r10.html

namespace mtl
{
    namespace expected_impl
    {
        using std::in_place_t, std::initializer_list;

        template <class E>
        class unexpected
        {
        public:
            constexpr unexpected(const unexpected&) = default;
            constexpr unexpected(unexpected&&) = default;

            template <class... Args>
            constexpr explicit unexpected(in_place_t, Args&&...);
            template <class U, class... Args>
            constexpr explicit unexpected(in_place_t, initializer_list<U>, Args&&...);
            template <class Err = E>
            constexpr explicit unexpected(Err&& e) : _value(std::move(e))
            {}
            template <class Err>
            /*explicit*/ constexpr unexpected(const unexpected<Err>& e) : _value(e)
            {}
            template <class Err>
            /*explicit*/ constexpr unexpected(unexpected<Err>&& e) : _value(std::move(e))
            {}

            constexpr unexpected& operator=(const unexpected&) = default;
            constexpr unexpected& operator=(unexpected&&) = default;
            template <class Err = E>
            constexpr unexpected& operator=(const unexpected<Err>&);
            template <class Err = E>
            constexpr unexpected& operator=(unexpected<Err>&&);

            constexpr const E& value() const& noexcept { return _value; }
            constexpr E& value() & noexcept { return _value; }
            constexpr const E&& value() const&& noexcept { return std::move(_value); }
            constexpr E&& value() && noexcept { return std::move(_value); }

            void swap(unexpected& other) noexcept
            {
                using std::swap;
                swap(_value, other._value);
            }

            template <class E1, class E2>
            friend constexpr bool operator==(const unexpected<E1>& x, const unexpected<E2>& y);
            template <class E1, class E2>
            friend constexpr bool operator!=(const unexpected<E1>& x, const unexpected<E2>& y);
            template <class E1>
            friend void swap(unexpected<E1>& x, unexpected<E1>& y) noexcept;

        private:
            E _value;
        };

        template <class E1, class E2>
        constexpr bool operator==(const unexpected<E1>& x, const unexpected<E2>& y)
        {
            return x._value == y._value;
        }

        template <class E1, class E2>
        constexpr bool operator!=(const unexpected<E1>& x, const unexpected<E2>& y)
        {
            return !(x == y);
        }

        template <class E1>
        void swap(unexpected<E1>& x, unexpected<E1>& y) noexcept
        {
            x.swap(y);
        }

        template <class E>
        unexpected(E) -> unexpected<E>;

        struct unexpect_t
        {
            explicit unexpect_t() = default;
        };
        inline constexpr unexpect_t unexpect{};

        template <class T, class E>
        class expected
        {
        public:
            using value_type = T;
            using error_type = E;
            using unexpected_type = unexpected<E>;

            template <class U>
            using rebind = expected<U, error_type>;

            // ?.?.4.1, constructors
            constexpr expected() : _has_value(true) { new (&_value) T(); }
            constexpr expected(const expected& rhs) : _has_value(rhs._has_value)
            {
                if (_has_value)
                    new (&_value) value_type(rhs._value);
                else
                    new (&_error) unexpected_type(rhs._error);
            }
            constexpr expected(expected&& rhs) : _has_value(rhs._has_value)
            {
                if (_has_value)
                    new (&_value) value_type(std::move(rhs._value));
                else
                    new (&_error) unexpected_type(std::move(rhs._error));
            }
            template <class U, class G>
            /*explicit*/ constexpr expected(const expected<U, G>&);
            template <class U, class G>
            /*explicit*/ constexpr expected(expected<U, G>&&);
            template <class U = T>
            /*explicit*/ constexpr expected(U&& value) : _has_value(true)
            {
                new (&_value) value_type(std::forward<U>(value));
            }

            template <class G = E>
            constexpr expected(const unexpected<G>& error) : _has_value(false)
            {
                new (&_error) unexpected_type(error);
            }
            template <class G = E>
            constexpr expected(unexpected<G>&& error) : _has_value(false)
            {
                new (&_error) unexpected_type(std::move(error));
            }

            template <class... Args>
            constexpr explicit expected(in_place_t, Args&&...);
            template <class U, class... Args>
            constexpr explicit expected(in_place_t, initializer_list<U>, Args&&...);
            template <class... Args>
            constexpr explicit expected(unexpect_t, Args&&...);
            template <class U, class... Args>
            constexpr explicit expected(unexpect_t, initializer_list<U>, Args&&...);

            // ?.?.4.2, destructor
            ~expected()
            {
                if (_has_value)
                    _value.~value_type();
                else
                    _error.~unexpected_type();
            }

            // ?.?.4.3, assignment
            expected& operator=(const expected& rhs)
            {
                if (rhs._has_value)
                    *this = rhs._value;
                else
                    *this = rhs._error;
                return *this;
            }
            expected& operator=(expected&& rhs) noexcept
            {
                if (rhs._has_value)
                    *this = std::move(rhs._value);
                else
                    *this = std::move(rhs._error);
                return *this;
            }
            template <class U = T>
            expected& operator=(U&& rhs)
            {
                if (_has_value)
                {
                    _value = std::forward<U>(rhs);
                }
                else
                {
                    _error.~unexpected_type();
                    new (&_value) value_type(std::forward<U>(rhs));
                    _has_value = true;
                }
                return *this;
            }
            template <class G = E>
            expected& operator=(const unexpected<G>& rhs)
            {
                if (_has_value)
                {
                    _value.~value_type();
                    new (&_error) unexpected_type(rhs);
                    _has_value = false;
                }
                else
                {
                    _error = rhs;
                }
                return *this;
            }
            template <class G = E>
            expected& operator=(unexpected<G>&& rhs)
            {
                if (_has_value)
                {
                    _value.~value_type();
                    new (&_error) unexpected_type(std::move(rhs));
                    _has_value = false;
                }
                else
                {
                    _error = std::move(rhs);
                }
                return *this;
            }

            // ?.?.4.4, modifiers

            template <class... Args>
            T& emplace(Args&&...);
            template <class U, class... Args>
            T& emplace(initializer_list<U>, Args&&...);

            // ?.?.4.5, swap
            void swap(expected& other) noexcept
            {
                if (_has_value)
                {
                    if (other._has_value)
                    {
                        using std::swap;
                        swap(_value, other._value);
                    }
                    else
                    {
                        value_type tmp(std::move(_value));
                        _value.~value_type();
                        new (&_error) unexpected_type(std::move(other._error));
                        _has_value = false;
                        other._error.~unexpected_type();
                        new (&other._value) value_type(std::move(tmp));
                        other._has_value = true;
                    }
                }
                else
                {
                    if (other._has_value)
                    {
                        other.swap(*this);
                    }
                    else
                    {
                        using std::swap;
                        swap(_error, other._error);
                    }
                }
            }

            // ?.?.4.6, observers
            constexpr const T* operator->() const { return _value; }
            constexpr T* operator->() { return _value; }
            constexpr const T& operator*() const& { return _value; }
            constexpr T& operator*() & { return _value; }
            constexpr const T&& operator*() const&& { return std::move(_value); }
            constexpr T&& operator*() && { return std::move(_value); };
            constexpr explicit operator bool() const noexcept { return _has_value; }
            constexpr bool has_value() const noexcept { return _has_value; }
            constexpr const T& value() const&
            {
                assert(has_value);
                return _value;
            }
            constexpr T& value() &
            {
                assert(has_value);
                return _value;
            }
            constexpr const T&& value() const&&
            {
                assert(has_value);
                return std::move(_value);
            }
            constexpr T&& value() &&
            {
                assert(has_value);
                return std::move(_value);
            }
            constexpr const E& error() const&
            {
                assert(!_has_value);
                return _error.value();
            }
            constexpr E& error() &
            {
                assert(!_has_value);
                return _error.value();
            }
            constexpr const E&& error() const&&
            {
                assert(!_has_value);
                return std::move(_error.value());
            }
            constexpr E&& error() &&
            {
                assert(!_has_value);
                return std::move(_error.value());
            }
            template <class U>
            constexpr T value_or(U&& value) const&
            {
                return _has_value ? _value : static_cast<T>(std::forward<U>(value));
            }
            template <class U>
            constexpr T value_or(U&& value) &&
            {
                return _has_value ? std::move(_value) : static_cast<T>(std::forward<U>(value));
            }

            // ?.?.4.7, Expected equality operators
            template <class T1, class E1, class T2, class E2>
            friend constexpr bool operator==(const expected<T1, E1>& x, const expected<T2, E2>& y);
            template <class T1, class E1, class T2, class E2>
            friend constexpr bool operator!=(const expected<T1, E1>& x, const expected<T2, E2>& y);

            // ?.?.4.8, Comparison with T
            template <class T1, class E1, class T2>
            friend constexpr bool operator==(const expected<T1, E1>& x, const T2& y);
            template <class T1, class E1, class T2>
            friend constexpr bool operator==(const T2& x, const expected<T1, E1>& y);
            template <class T1, class E1, class T2>
            friend constexpr bool operator!=(const expected<T1, E1>& x, const T2& y);
            template <class T1, class E1, class T2>
            friend constexpr bool operator!=(const T2& x, const expected<T1, E1>& y);

            // ?.?.4.9, Comparison with unexpected<E>
            template <class T1, class E1, class E2>
            friend constexpr bool operator==(const expected<T1, E1>& x, const unexpected<E2>& y);
            template <class T1, class E1, class E2>
            friend constexpr bool operator==(const unexpected<E2>& x, const expected<T1, E1>& y);
            template <class T1, class E1, class E2>
            friend constexpr bool operator!=(const expected<T1, E1>& x, const unexpected<E2>& y);
            template <class T1, class E1, class E2>
            friend constexpr bool operator!=(const unexpected<E2>& x, const expected<T1, E1>& y);

            // ?.?.4.10, Specialized algorithms
            template <class T1, class E1>
            friend void swap(expected<T1, E1>& x, expected<T1, E1>& y) noexcept;

        private:
            bool _has_value;
            union
            {
                value_type _value;
                unexpected_type _error;
            };
        };

        template <class T1, class E1, class T2, class E2>
        constexpr bool operator==(const expected<T1, E1>& x, const expected<T2, E2>& y)
        {
            if (x._has_value != y._has_value)
                return false;
            else
                return (x._has_value) ? x._value == y._value : x._error == y._error;
        }
        template <class T1, class E1, class T2, class E2>
        constexpr bool operator!=(const expected<T1, E1>& x, const expected<T2, E2>& y)
        {
            return !(x == y);
        }

        template <class T1, class E1, class T2>
        constexpr bool operator==(const expected<T1, E1>& x, const T2& y)
        {
            return x._has_value && x._value == y;
        }
        template <class T1, class E1, class T2>
        constexpr bool operator==(const T2& x, const expected<T1, E1>& y)
        {
            return y == x;
        }
        template <class T1, class E1, class T2>
        constexpr bool operator!=(const expected<T1, E1>& x, const T2& y)
        {
            return !(x == y);
        }
        template <class T1, class E1, class T2>
        constexpr bool operator!=(const T2& x, const expected<T1, E1>& y)
        {
            return !(x == y);
        }

        template <class T1, class E1, class E2>
        constexpr bool operator==(const expected<T1, E1>& x, const unexpected<E2>& y)
        {
            return !x._has_value && x._error == y;
        }
        template <class T1, class E1, class E2>
        constexpr bool operator==(const unexpected<E2>& x, const expected<T1, E1>& y)
        {
            return y == x;
        }
        template <class T1, class E1, class E2>
        constexpr bool operator!=(const expected<T1, E1>& x, const unexpected<E2>& y)
        {
            return !(x == y);
        }
        template <class T1, class E1, class E2>
        constexpr bool operator!=(const unexpected<E2>& x, const expected<T1, E1>& y)
        {
            return !(x == y);
        }

        template <class T1, class E1>
        void swap(expected<T1, E1>& x, expected<T1, E1>& y) noexcept
        {
            x.swap(y);
        }

    } // namespace expected_impl

    using namespace expected_impl;

} // namespace mtl
