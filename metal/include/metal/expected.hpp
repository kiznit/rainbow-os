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

#pragma once

#include <cassert>
#include <functional>
#include <memory>
#include <metal/type_traits.hpp>
#include <metal/unexpected.hpp>

namespace mtl
{
    namespace detail
    {
        using std::construct_at;
        using std::destroy_at;

        template <class T, class U, class... Args>
        constexpr void reinit_expected(T& newval, U& oldval, Args&&... args)
        {
            detail::destroy_at(std::addressof(oldval));
            detail::construct_at(std::addressof(newval), std::forward<Args>(args)...);
        }
    } // namespace detail

    /*
        expected
    */

    template <class T, class E>
    class expected
    {
    public:
        using value_type = T;
        using error_type = E;
        using unexpected_type = unexpected<E>;

        template <class U>
        using rebind = expected<U, error_type>;

        // Constructors
        constexpr expected()
            requires(std::is_default_constructible_v<T>)
            : _value(), _has_value(true)
        {
        }

        constexpr expected(const expected& rhs) noexcept(std::is_nothrow_copy_constructible_v<T> &&
                                                         std::is_nothrow_copy_constructible_v<E>)
            requires(std::is_copy_constructible_v<T> && std::is_copy_constructible_v<E> &&
                     !(std::is_trivially_copy_constructible_v<T> && std::is_trivially_copy_constructible_v<E>))
        {
            if (rhs._has_value)
            {
                construct_value(rhs._value);
            }
            else
            {
                construct_error(rhs._error);
            }
        }
        constexpr expected(const expected&) noexcept
            requires(std::is_trivially_copy_constructible_v<T> && std::is_trivially_copy_constructible_v<E>)
        = default;

        constexpr expected(const expected&)
            requires(!std::is_copy_constructible_v<T> || !std::is_copy_constructible_v<E>)
        = delete;

        constexpr expected(expected&& rhs) noexcept(std::is_nothrow_move_constructible_v<T> &&
                                                    std::is_nothrow_move_constructible_v<E>)
            requires(std::is_move_constructible_v<T> && std::is_move_constructible_v<E> &&
                     !(std::is_trivially_move_constructible_v<T> && std::is_trivially_move_constructible_v<E>))
        {
            if (rhs._has_value)
            {
                construct_value(std::move(rhs._value));
            }
            else
            {
                construct_error(std::move(rhs._error));
            }
        }

        constexpr expected(expected&&) noexcept
            requires(std::is_trivially_move_constructible_v<T> && std::is_trivially_move_constructible_v<E>)
        = default;

        template <class U, class G>
        constexpr explicit(!std::is_convertible_v<const U&, T> || !std::is_convertible_v<const G&, E>)
            expected(const expected<U, G>& rhs)
            requires(std::is_constructible_v<T, const U&> && std::is_constructible_v<E, const G&> &&
                     !std::is_constructible_v<T, expected<U, G>&> && !std::is_constructible_v<T, expected<U, G>> &&
                     !std::is_constructible_v<T, const expected<U, G>&> && !std::is_constructible_v<T, const expected<U, G>> &&
                     !std::is_convertible_v<expected<U, G>&, T> && !std::is_convertible_v<expected<U, G> &&, T> &&
                     !std::is_convertible_v<const expected<U, G>&, T> && !std::is_convertible_v<const expected<U, G> &&, T> &&
                     !std::is_constructible_v<unexpected<E>, expected<U, G>&> &&
                     !std::is_constructible_v<unexpected<E>, expected<U, G>> &&
                     !std::is_constructible_v<unexpected<E>, const expected<U, G>&> &&
                     !std::is_constructible_v<unexpected<E>, const expected<U, G>>)
        {
            if (rhs.has_value())
            {
                construct_value(std::forward<const U&>(*rhs));
            }
            else
            {
                construct_error(std::forward<const G&>(rhs.error()));
            }
        }

        template <class U, class G>
        constexpr explicit(!std::is_convertible_v<U, T> || !std::is_convertible_v<G, E>) expected(expected<U, G>&& rhs)
            requires(std::is_constructible_v<T, U> && std::is_constructible_v<E, G> &&
                     !std::is_constructible_v<T, expected<U, G>&> && !std::is_constructible_v<T, expected<U, G>> &&
                     !std::is_constructible_v<T, const expected<U, G>&> && !std::is_constructible_v<T, const expected<U, G>> &&
                     !std::is_convertible_v<expected<U, G>&, T> && !std::is_convertible_v<expected<U, G> &&, T> &&
                     !std::is_convertible_v<const expected<U, G>&, T> && !std::is_convertible_v<const expected<U, G> &&, T> &&
                     !std::is_constructible_v<unexpected<E>, expected<U, G>&> &&
                     !std::is_constructible_v<unexpected<E>, expected<U, G>> &&
                     !std::is_constructible_v<unexpected<E>, const expected<U, G>&> &&
                     !std::is_constructible_v<unexpected<E>, const expected<U, G>>)
        {
            if (rhs.has_value())
            {
                construct_value(std::forward<U>(*rhs));
            }
            else
            {
                construct_error(std::forward<G>(rhs.error()));
            }
        }

        template <class U = T>
        constexpr explicit(!std::is_convertible_v<U, T>) expected(U&& v)
            requires(!std::is_same_v<std::remove_cvref_t<U>, in_place_t> &&
                     !std::is_same_v<expected<T, E>, std::remove_cvref_t<U>> &&
                     !mtl::is_specialization<std::remove_cvref_t<U>, unexpected>::value && std::is_constructible_v<T, U>)
            : _value(std::forward<U>(v)), _has_value(true)
        {
        }

        template <class G>
        constexpr explicit(!std::is_convertible_v<const G&, E>) expected(const unexpected<G>& e)
            requires(std::is_constructible_v<E, const G&>)
            : _error(std::forward<const G&>(e.value())), _has_value(false)
        {
        }

        template <class G>
        constexpr explicit(!std::is_convertible_v<G, E>) expected(unexpected<G>&& e)
            requires(std::is_constructible_v<E, G>)
            : _error(std::forward<G>(e.value())), _has_value(false)
        {
        }

        template <class... Args>
        constexpr explicit expected(in_place_t, Args&&... args)
            requires(std::is_constructible_v<T, Args...>)
            : _value(std::forward<Args>(args)...), _has_value(true)
        {
        }

        template <class U, class... Args>
        constexpr explicit expected(in_place_t, initializer_list<U> il, Args&&... args)
            requires(std::is_constructible_v<T, initializer_list<U>&, Args...>)
            : _value(il, std::forward<Args>(args)...), _has_value(true)
        {
        }

        template <class... Args>
        constexpr explicit expected(unexpect_t, Args&&... args)
            requires(std::is_constructible_v<E, Args...>)
            : _error(std::forward<Args>(args)...), _has_value(false)
        {
        }

        template <class U, class... Args>
        constexpr explicit expected(unexpect_t, initializer_list<U> il, Args&&... args)
            requires(std::is_constructible_v<E, initializer_list<U>&, Args...>)
            : _error(il, std::forward<Args>(args)...), _has_value(false)
        {
        }

        // Destructor
        constexpr ~expected()
        {
            if (_has_value)
            {
                detail::destroy_at(std::addressof(_value));
            }
            else
            {
                detail::destroy_at(std::addressof(_error));
            }
        }

        constexpr ~expected()
            requires(std::is_trivially_destructible_v<T> && !std::is_trivially_destructible_v<E>)
        {
            if (!_has_value)
            {
                detail::destroy_at(std::addressof(_error));
            }
        }

        constexpr ~expected()
            requires(!std::is_trivially_destructible_v<T> && std::is_trivially_destructible_v<E>)
        {
            if (_has_value)
            {
                detail::destroy_at(std::addressof(_value));
            }
        }

        constexpr ~expected()
            requires(std::is_trivially_destructible_v<T> && std::is_trivially_destructible_v<E>)
        = default;

        // Assignment
        constexpr expected& operator=(const expected& rhs) noexcept(std::is_nothrow_copy_assignable_v<T> &&
                                                                    std::is_nothrow_copy_constructible_v<T> &&
                                                                    std::is_nothrow_copy_assignable_v<E> &&
                                                                    std::is_nothrow_copy_constructible_v<E>)
            requires(std::is_copy_assignable_v<T> && std::is_copy_constructible_v<T> && std::is_copy_assignable_v<E> &&
                     std::is_copy_constructible_v<E> &&
                     (std::is_nothrow_move_constructible_v<T> || std::is_nothrow_move_constructible_v<E>))
        {
            if (_has_value)
            {
                if (rhs._has_value)
                {
                    _value = rhs._value;
                }
                else
                {
                    detail::reinit_expected(_error, _value, rhs._error);
                    _has_value = false;
                }
            }
            else
            {
                if (rhs._has_value)
                {
                    detail::reinit_expected(_value, _error, rhs._value);
                    _has_value = true;
                }
                else
                {
                    _error = rhs._error;
                }
            }
            return *this;
        }

        constexpr expected& operator=(const expected&) = delete;

        constexpr expected& operator=(expected&& rhs) noexcept(std::is_nothrow_move_assignable_v<T> &&
                                                               std::is_nothrow_move_constructible_v<T> &&
                                                               std::is_nothrow_move_assignable_v<E> &&
                                                               std::is_nothrow_move_constructible_v<E>)
            requires(std::is_move_constructible_v<T> && std::is_move_assignable_v<T> && std::is_move_constructible_v<E> &&
                     std::is_move_assignable_v<E> &&
                     (std::is_nothrow_move_constructible_v<T> || std::is_nothrow_move_constructible_v<E>))
        {
            if (_has_value)
            {
                if (rhs._has_value)
                {
                    _value = std::move(rhs._value);
                }
                else
                {
                    detail::reinit_expected(_error, _value, std::move(rhs._error));
                    _has_value = false;
                }
            }
            else
            {
                if (rhs._has_value)
                {
                    detail::reinit_expected(_value, _error, std::move(rhs._value));
                    _has_value = true;
                }
                else
                {
                    _error = std::move(rhs._error);
                }
            }
            return *this;
        }

        constexpr expected& operator=(expected&&) = delete;

        template <class U = T>
        constexpr expected& operator=(U&& v)
            requires(!std::is_same_v<expected, std::remove_cvref_t<U>> &&
                     !mtl::is_specialization<std::remove_cvref_t<U>, unexpected>::value && std::is_constructible_v<T, U> &&
                     std::is_assignable_v<T&, U> &&
                     (std::is_nothrow_constructible_v<T, U> || std::is_nothrow_move_constructible_v<T> ||
                      std::is_nothrow_move_constructible_v<E>))
        {
            if (_has_value)
            {
                _value = std::forward<U>(v);
            }
            else
            {
                detail::reinit_expected(_value, _error, std::forward<U>(v));
                _has_value = true;
            }
            return *this;
        }

        template <class G>
        constexpr expected& operator=(const unexpected<G>& e)
            requires(std::is_constructible_v<E, const G&> && std::is_assignable_v<E&, const G&> &&
                     (std::is_nothrow_constructible_v<E, const G&> || std::is_nothrow_move_constructible_v<T> ||
                      std::is_nothrow_move_constructible_v<E>))
        {
            if (_has_value)
            {
                detail::reinit_expected(_error, _value, std::forward<const G&>(e.value()));
                _has_value = false;
            }
            else
            {
                _error = std::forward<const G&>(e.value());
            }
            return *this;
        }

        template <class G>
        constexpr expected& operator=(unexpected<G>&& e)
            requires(std::is_constructible_v<E, G> && std::is_assignable_v<E&, G> &&
                     (std::is_nothrow_constructible_v<E, G> || std::is_nothrow_move_constructible_v<T> ||
                      std::is_nothrow_move_constructible_v<E>))
        {
            if (_has_value)
            {
                detail::reinit_expected(_error, _value, std::forward<G>(e.value()));
                _has_value = false;
            }
            else
            {
                _error = std::forward<G>(e.value());
            }
            return *this;
        }

        // Modifiers
        template <class... Args>
        constexpr T& emplace(Args&&... args) noexcept
            requires(std::is_nothrow_constructible_v<T, Args...>)
        {
            if (_has_value)
                detail::destroy_at(std::addressof(_value));
            else
            {
                detail::destroy_at(std::addressof(_error));
            }
            construct_value(std::forward<Args>(args)...);
            return _value;
        }

        template <class U, class... Args>
        constexpr T& emplace(initializer_list<U> il, Args&&... args) noexcept
            requires(std::is_nothrow_constructible_v<T, std::initializer_list<U>&, Args...>)
        {
            if (_has_value)
                detail::destroy_at(std::addressof(_value));
            else
            {
                detail::destroy_at(std::addressof(_error));
            }
            construct_value(il, std::forward<Args>(args)...);
            return _value;
        }

        // Swap
        constexpr void swap(expected& rhs) noexcept(std::is_nothrow_move_constructible_v<T> && std::is_nothrow_swappable_v<T> &&
                                                    std::is_nothrow_move_constructible_v<E> && std::is_nothrow_swappable_v<E>)
            requires(std::is_swappable_v<T> && std::is_swappable_v<E> && std::is_move_constructible_v<T> &&
                     std::is_move_constructible_v<E> &&
                     (std::is_nothrow_move_constructible_v<T> || std::is_nothrow_move_constructible_v<E>))
        {
            if (_has_value)
            {
                if (rhs._has_value)
                {
                    using std::swap;
                    std::swap(_value, rhs._value);
                }
                else
                {
                    E tmp(std::move(rhs._error));
                    detail::destroy_at(std::addressof(rhs._error));
                    rhs.construct_value(std::move(_value));
                    detail::destroy_at(std::addressof(_value));
                    construct_error(std::move(tmp));
                }
            }
            else
            {
                if (rhs._has_value)
                {
                    rhs.swap(*this);
                }
                else
                {
                    using std::swap;
                    std::swap(_error, rhs._error);
                }
            }
        }

        friend constexpr void swap(expected& x, expected& y) noexcept(noexcept(x.swap(y))) { x.swap(y); }

        // Observers
        constexpr const T* operator->() const noexcept { return std::addressof(_value); }
        constexpr T* operator->() noexcept { return std::addressof(_value); }
        constexpr const T& operator*() const& { return _value; }
        constexpr T& operator*() & { return _value; }
        constexpr const T&& operator*() const&& { return std::move(_value); }
        constexpr T&& operator*() && { return std::move(_value); }
        constexpr explicit operator bool() const noexcept { return _has_value; }
        constexpr bool has_value() const noexcept { return _has_value; }

        constexpr const T& value() const&
        {
            assert(_has_value);
            return _value;
        }

        constexpr T& value() &
        {
            assert(_has_value);
            return _value;
        }

        constexpr const T&& value() const&&
        {
            assert(_has_value);
            return std::move(_value);
        }

        constexpr T&& value() &&
        {
            assert(_has_value);
            return std::move(_value);
        }

        constexpr const E& error() const& { return _error; }
        constexpr E& error() & { return _error; }
        constexpr const E&& error() const&& { return std::move(_error); }
        constexpr E&& error() && { return std::move(_error); }

        template <class U>
        constexpr T value_or(U&& v) const&
        {
            return _has_value ? _value : static_cast<T>(std::forward<U>(v));
        }

        template <class U>
        constexpr T value_or(U&& v) &&
        {
            return _has_value ? std::move(_value) : static_cast<T>(std::forward<U>(v));
        }

        template <class G = E>
        constexpr E error_or(G&& e) const&
        {
            return _has_value ? std::forward<G>(e) : _error;
        }

        template <class G = E>
        constexpr E error_or(G&& e) &&
        {
            return _has_value ? std::forward<G>(e) : std::move(_error);
        }

        template <class F>
            requires(std::is_copy_constructible_v<E>)
        constexpr auto and_then(F&& f) &
        {
            using U = std::remove_cvref_t<std::invoke_result_t<F, decltype(value())>>;
            return _has_value ? std::invoke(std::forward<F>(f), value()) : U(unexpect, error());
        }

        template <class F>
            requires(std::is_copy_constructible_v<E>)
        constexpr auto and_then(F&& f) const&
        {
            using U = std::remove_cvref_t<std::invoke_result_t<F, decltype(value())>>;
            return _has_value ? std::invoke(std::forward<F>(f), value()) : U(unexpect, error());
        }

        template <class F>
            requires(std::is_move_constructible_v<E>)
        constexpr auto and_then(F&& f) &&
        {
            using U = std::remove_cvref_t<std::invoke_result_t<F, decltype(std::move(value()))>>;
            return _has_value ? std::invoke(std::forward<F>(f), std::move(value())) : U(unexpect, std::move(error()));
        }

        template <class F>
            requires(std::is_move_constructible_v<E>)
        constexpr auto and_then(F&& f) const&&
        {
            using U = std::remove_cvref_t<std::invoke_result_t<F, decltype(std::move(value()))>>;
            return _has_value ? std::invoke(std::forward<F>(f), std::move(value())) : U(unexpect, std::move(error()));
        }

        template <class F>
            requires(std::is_copy_constructible_v<T>)
        constexpr auto or_else(F&& f) &
        {
            using G = std::remove_cvref_t<std::invoke_result_t<F, decltype(error())>>;
            return _has_value ? G(std::in_place, value()) : std::invoke(std::forward<F>(f), error());
        }

        template <class F>
            requires(std::is_copy_constructible_v<T>)
        constexpr auto or_else(F&& f) const&
        {
            using G = std::remove_cvref_t<std::invoke_result_t<F, decltype(error())>>;
            return _has_value ? G(std::in_place, value()) : std::invoke(std::forward<F>(f), error());
        }

        template <class F>
            requires(std::is_move_constructible_v<T>)
        constexpr auto or_else(F&& f) &&
        {
            using G = std::remove_cvref_t<std::invoke_result_t<F, decltype(std::move(error()))>>;
            return _has_value ? G(std::in_place, std::move(value())) : std::invoke(std::forward<F>(f), std::move(error()));
        }

        template <class F>
            requires(std::is_move_constructible_v<T>)
        constexpr auto or_else(F&& f) const&&
        {
            using G = std::remove_cvref_t<std::invoke_result_t<F, decltype(std::move(error()))>>;
            return _has_value ? G(std::in_place, std::move(value())) : std::invoke(std::forward<F>(f), std::move(error()));
        }

        template <class F>
            requires(std::is_copy_constructible_v<E>)
        constexpr auto transform(F&& f) &
        {
            using U = std::remove_cvref_t<std::invoke_result_t<F, decltype(value())>>;

            if (!_has_value)
                return expected<U, E>(unexpect, error());

            if constexpr (!std::is_void_v<U>)
                return expected<U, E>(std::invoke(std::forward<F>(f), value()));
            else
            {
                std::invoke(std::forward<F>(f), value());
                return expected<U, E>();
            }
        }

        template <class F>
            requires(std::is_copy_constructible_v<E>)
        constexpr auto transform(F&& f) const&
        {
            using U = std::remove_cvref_t<std::invoke_result_t<F, decltype(value())>>;

            if (!_has_value)
                return expected<U, E>(unexpect, error());

            if constexpr (!std::is_void_v<U>)
                return expected<U, E>(std::invoke(std::forward<F>(f), value()));
            else
            {
                std::invoke(std::forward<F>(f), value());
                return expected<U, E>();
            }
        }

        template <class F>
            requires(std::is_move_constructible_v<E>)
        constexpr auto transform(F&& f) &&
        {
            using U = std::remove_cvref_t<std::invoke_result_t<F, decltype(std::move(value()))>>;

            if (!_has_value)
                return expected<U, E>(unexpect, std::move(error()));

            if constexpr (!std::is_void_v<U>)
                return expected<U, E>(std::invoke(std::forward<F>(f), std::move(value())));
            else
            {
                std::invoke(std::forward<F>(f), std::move(value()));
                return expected<U, E>();
            }
        }

        template <class F>
            requires(std::is_move_constructible_v<E>)
        constexpr auto transform(F&& f) const&&
        {
            using U = std::remove_cvref_t<std::invoke_result_t<F, decltype(std::move(value()))>>;

            if (!_has_value)
                return expected<U, E>(unexpect, std::move(error()));

            if constexpr (!std::is_void_v<U>)
                return expected<U, E>(std::invoke(std::forward<F>(f), std::move(value())));
            else
            {
                std::invoke(std::forward<F>(f), std::move(value()));
                return expected<U, E>();
            }
        }

        template <class F>
            requires(std::is_copy_constructible_v<T>)
        constexpr auto transform_error(F&& f) &
        {
            using G = std::remove_cvref_t<std::invoke_result_t<F, decltype(error())>>;
            return _has_value ? expected<T, G>() : expected<T, G>(unexpect, std::invoke(std::forward<F>(f), error()));
        }

        template <class F>
            requires(std::is_copy_constructible_v<T>)
        constexpr auto transform_error(F&& f) const&
        {
            using G = std::remove_cvref_t<std::invoke_result_t<F, decltype(error())>>;
            return _has_value ? expected<T, G>() : expected<T, G>(unexpect, std::invoke(std::forward<F>(f), error()));
        }

        template <class F>
            requires(std::is_move_constructible_v<T>)
        constexpr auto transform_error(F&& f) &&
        {
            using G = std::remove_cvref_t<std::invoke_result_t<F, decltype(std::move(error()))>>;
            return _has_value ? expected<T, G>() : expected<T, G>(unexpect, std::invoke(std::forward<F>(f), std::move(error())));
        }

        template <class F>
            requires(std::is_move_constructible_v<T>)
        constexpr auto transform_error(F&& f) const&&
        {
            using G = std::remove_cvref_t<std::invoke_result_t<F, decltype(std::move(error()))>>;
            return _has_value ? expected<T, G>() : expected<T, G>(unexpect, std::invoke(std::forward<F>(f), std::move(error())));
        }

        // Equality operators
        template <class T2, class E2>
            requires(!std::is_void_v<T2>)
        friend constexpr bool operator==(const expected& x, const expected<T2, E2>& y)
        {
            if (x.has_value())
                return y.has_value() && static_cast<bool>(x.value() == y.value());
            else
                return !y.has_value() && static_cast<bool>(x.error() == y.error());
        }

        template <class T2>
        friend constexpr bool operator==(const expected& x, const T2& v)
        {
            return x.has_value() && static_cast<bool>(*x == v);
        }

        template <class E2>
        friend constexpr bool operator==(const expected& x, const unexpected<E2>& e)
        {
            return !x.has_value() && static_cast<bool>(x.error() == e.value());
        }

#if defined(__GNUC__) && __GNUC__ < 10 && !defined(__clang__)
        template <class T2, class E2>
            requires(!std::is_void_v<T2>)
        friend constexpr bool operator!=(const expected& x, const expected<T2, E2>& y)
        {
            return !(x == y);
        }

        template <class T2>
        friend constexpr bool operator!=(const expected& x, const T2& v)
        {
            return !(x == v);
        }

        template <class E2>
        friend constexpr bool operator!=(const expected& x, const unexpected<E2>& e)
        {
            return !(x == e);
        }
#endif

    private:
        template <class... Args>
        constexpr void construct_value(Args&&... args)
        {
            detail::construct_at(std::addressof(_value), std::forward<Args>(args)...);
            _has_value = true;
        }

        template <class... Args>
        constexpr void construct_error(Args&&... args)
        {
            detail::construct_at(std::addressof(_error), std::forward<Args>(args)...);
            _has_value = false;
        }

        union
        {
            T _value;
            E _error;
        };
        bool _has_value;
    };

    /*
        Partial specialization for void types
    */

    template <class T, class E>
        requires std::is_void_v<T>
    class expected<T, E>
    {
    public:
        using value_type = T;
        using error_type = E;
        using unexpected_type = unexpected<E>;

        template <class U>
        using rebind = expected<U, error_type>;

        // Constructors
        constexpr expected() noexcept : _has_value(true) {}

        constexpr expected(const expected& rhs)
            requires(std::is_copy_constructible_v<E> && !std::is_trivially_copy_constructible_v<E>)
        {
            if (rhs._has_value)
            {
                construct_value();
            }
            else
            {
                construct_error(rhs._error);
            }
        }

        constexpr expected(const expected&) noexcept
            requires(std::is_trivially_copy_constructible_v<E>)
        = default;

        constexpr expected(const expected&)
            requires(!std::is_copy_constructible_v<T> && !std::is_copy_constructible_v<E>)
        = delete;

        constexpr expected(expected&& rhs) noexcept(std::is_nothrow_move_constructible_v<E>)
            requires(std::is_move_constructible_v<E> && !std::is_trivially_move_constructible_v<E>)
        {
            if (rhs._has_value)
            {
                construct_value();
            }
            else
            {
                construct_error(std::move(rhs._error));
            }
        }

        constexpr expected(expected&&) noexcept
            requires(std::is_trivially_move_constructible_v<E>)
        = default;

        template <class U, class G>
        constexpr explicit(!std::is_convertible_v<const G&, E>) expected(const expected<U, G>& rhs)
            requires(std::is_void_v<U> && std::is_constructible_v<E, const G&> &&
                     !std::is_constructible_v<unexpected<E>, expected<U, G>&> &&
                     !std::is_constructible_v<unexpected<E>, expected<U, G>> &&
                     !std::is_constructible_v<unexpected<E>, const expected<U, G>&> &&
                     !std::is_constructible_v<unexpected<E>, const expected<U, G>>)
        {
            if (bool(rhs))
            {
                construct_value();
            }
            else
            {
                construct_error(std::forward<const G&>(rhs.error()));
            }
        }

        template <class U, class G>
        constexpr explicit(!std::is_convertible_v<G, E>) expected(expected<U, G>&& rhs)
            requires(std::is_void_v<U> && std::is_constructible_v<E, G> &&
                     !std::is_constructible_v<unexpected<E>, expected<U, G>&> &&
                     !std::is_constructible_v<unexpected<E>, expected<U, G>> &&
                     !std::is_constructible_v<unexpected<E>, const expected<U, G>&> &&
                     !std::is_constructible_v<unexpected<E>, const expected<U, G>>)
        {
            if (bool(rhs))
            {
                construct_value();
            }
            else
            {
                construct_error(std::forward<G>(rhs.error()));
            }
        }

        template <class G>
        constexpr explicit(!std::is_convertible_v<const G&, E>) expected(const unexpected<G>& e)
            requires(std::is_constructible_v<E, const G&>)
            : _error(std::forward<const G&>(e.value())), _has_value(false)
        {
        }

        template <class G>
        constexpr explicit(!std::is_convertible_v<G, E>) expected(unexpected<G>&& e)
            requires(std::is_constructible_v<E, G>)
            : _error(std::forward<G>(e.value())), _has_value(false)
        {
        }

        constexpr explicit expected(in_place_t) noexcept : _has_value(true) {}

        template <class... Args>
        constexpr explicit expected(unexpect_t, Args&&... args)
            requires(std::is_constructible_v<E, Args...>)
            : _error(std::forward<Args>(args)...), _has_value(false)
        {
        }

        template <class U, class... Args>
        constexpr explicit expected(unexpect_t, initializer_list<U> il, Args&&... args)
            requires(std::is_constructible_v<E, initializer_list<U>&, Args...>)
            : _error(il, std::forward<Args>(args)...), _has_value(false)
        {
        }

        // Destructor
        constexpr ~expected()
        {
            if (!_has_value)
            {
                _error.~E();
            }
        }

        constexpr ~expected()
            requires(std::is_trivially_destructible_v<E>)
        = default;

        // Assignment
        constexpr expected& operator=(const expected& rhs) noexcept(std::is_nothrow_copy_assignable_v<E> &&
                                                                    std::is_nothrow_copy_constructible_v<E>)
            requires(std::is_copy_assignable_v<E> && std::is_copy_constructible_v<E>)
        {
            if (_has_value)
            {
                if (!rhs._has_value)
                {
                    construct_error(rhs._error);
                }
            }
            else
            {
                if (rhs._has_value)
                {
                    detail::destroy_at(std::addressof(_error));
                    construct_value();
                }
                else
                {
                    _error = rhs._error;
                }
            }
            return *this;
        }

        constexpr expected& operator=(const expected&) = delete;

        constexpr expected& operator=(expected&& rhs) noexcept(std::is_nothrow_move_assignable_v<E> &&
                                                               std::is_nothrow_move_constructible_v<E>)
            requires(std::is_move_constructible_v<E> && std::is_move_assignable_v<E>)
        {
            if (_has_value)
            {
                if (!rhs._has_value)
                {
                    construct_error(std::move(rhs._error));
                }
            }
            else
            {
                if (rhs._has_value)
                {
                    detail::destroy_at(std::addressof(_error));
                    construct_value();
                }
                else
                {
                    _error = std::move(rhs._error);
                }
            }
            return *this;
        }

        template <class G>
        constexpr expected& operator=(const unexpected<G>& e)
            requires(std::is_constructible_v<E, const G&> && std::is_assignable_v<E&, const G&>)
        {
            if (_has_value)
            {
                construct_error(std::forward<const G&>(e.value()));
            }
            else
            {
                _error = std::forward<const G&>(e.value());
            }
            return *this;
        }

        template <class G>
        constexpr expected& operator=(unexpected<G>&& e)
            requires(std::is_constructible_v<E, G> && std::is_assignable_v<E&, G>)
        {
            if (_has_value)
            {
                construct_error(std::forward<G>(e.value()));
            }
            else
            {
                _error = std::forward<G>(e.value());
            }
            return *this;
        }

        // Modifiers
        constexpr void emplace() noexcept
        {
            if (!_has_value)
            {
                detail::destroy_at(std::addressof(_error));
                _has_value = true;
            }
        }

        // Swap
        constexpr void swap(expected& rhs) noexcept(std::is_nothrow_move_constructible_v<E> && std::is_nothrow_swappable_v<E>)
            requires(std::is_swappable_v<E> && std::is_move_constructible_v<E>)
        {
            if (_has_value)
            {
                if (!rhs._has_value)
                {
                    construct_error(std::move(rhs._error));
                    detail::destroy_at(std::addressof(rhs._error));
                    rhs._has_value = true;
                }
            }
            else
            {
                if (rhs._has_value)
                {
                    rhs.swap(*this);
                }
                else
                {
                    using std::swap;
                    swap(_error, rhs._error);
                }
            }
        }

        friend constexpr void swap(expected& x, expected& y) noexcept(noexcept(x.swap(y))) { x.swap(y); }

        // Observers
        constexpr explicit operator bool() const noexcept { return _has_value; }
        constexpr bool has_value() const noexcept { return _has_value; }
        constexpr void operator*() const noexcept {}

        constexpr void value() const& { assert(_has_value); }

        constexpr void value() && { assert(_has_value); }

        constexpr const E& error() const& { return _error; }
        constexpr E& error() & { return _error; }
        constexpr const E&& error() const&& { return std::move(_error); }
        constexpr E&& error() && { return std::move(_error); }

        template <class G = E>
        constexpr E error_or(G&& e) const&
        {
            return _has_value ? std::forward<G>(e) : _error;
        }

        template <class G = E>
        constexpr E error_or(G&& e) &&
        {
            return _has_value ? std::forward<G>(e) : std::move(_error);
        }

        template <class F>
            requires(std::is_copy_constructible_v<E>)
        constexpr auto and_then(F&& f) &
        {
            using U = std::remove_cvref_t<std::invoke_result_t<F>>;
            return _has_value ? std::invoke(std::forward<F>(f)) : U(unexpect, error());
        }

        template <class F>
            requires(std::is_copy_constructible_v<E>)
        constexpr auto and_then(F&& f) const&
        {
            using U = std::remove_cvref_t<std::invoke_result_t<F>>;
            return _has_value ? std::invoke(std::forward<F>(f)) : U(unexpect, error());
        }

        template <class F>
            requires(std::is_move_constructible_v<E>)
        constexpr auto and_then(F&& f) &&
        {
            using U = std::remove_cvref_t<std::invoke_result_t<F>>;
            return _has_value ? std::invoke(std::forward<F>(f)) : U(unexpect, std::move(error()));
        }

        template <class F>
            requires(std::is_move_constructible_v<E>)
        constexpr auto and_then(F&& f) const&&
        {
            using U = std::remove_cvref_t<std::invoke_result_t<F>>;
            return _has_value ? std::invoke(std::forward<F>(f)) : U(unexpect, std::move(error()));
        }

        template <class F>
        constexpr auto or_else(F&& f) &
        {
            using G = std::remove_cvref_t<std::invoke_result_t<F, decltype(error())>>;
            return _has_value ? G(std::in_place, value()) : std::invoke(std::forward<F>(f), error());
        }

        template <class F>
        constexpr auto or_else(F&& f) const&
        {
            using G = std::remove_cvref_t<std::invoke_result_t<F, decltype(error())>>;
            return _has_value ? G(std::in_place, value()) : std::invoke(std::forward<F>(f), error());
        }

        template <class F>
        constexpr auto or_else(F&& f) &&
        {
            using G = std::remove_cvref_t<std::invoke_result_t<F, decltype(std::move(error()))>>;
            return _has_value ? G(std::in_place, std::move(value())) : std::invoke(std::forward<F>(f), std::move(error()));
        }

        template <class F>
        constexpr auto or_else(F&& f) const&&
        {
            using G = std::remove_cvref_t<std::invoke_result_t<F, decltype(std::move(error()))>>;
            return _has_value ? G(std::in_place, std::move(value())) : std::invoke(std::forward<F>(f), std::move(error()));
        }

        template <class F>
            requires(std::is_copy_constructible_v<E>)
        constexpr auto transform(F&& f) &
        {
            using U = std::remove_cvref_t<std::invoke_result_t<F>>;

            if (!_has_value)
                return expected<U, E>(unexpect, error());

            if constexpr (!std::is_void_v<U>)
                return expected<U, E>(std::invoke(std::forward<F>(f)));
            else
            {
                std::invoke(std::forward<F>(f));
                return expected<U, E>();
            }
        }

        template <class F>
            requires(std::is_copy_constructible_v<E>)
        constexpr auto transform(F&& f) const&
        {
            using U = std::remove_cvref_t<std::invoke_result_t<F>>;

            if (!_has_value)
                return expected<U, E>(unexpect, error());

            if constexpr (!std::is_void_v<U>)
                return expected<U, E>(std::invoke(std::forward<F>(f)));
            else
            {
                std::invoke(std::forward<F>(f));
                return expected<U, E>();
            }
        }

        template <class F>
            requires(std::is_move_constructible_v<E>)
        constexpr auto transform(F&& f) &&
        {
            using U = std::remove_cvref_t<std::invoke_result_t<F>>;

            if (!_has_value)
                return expected<U, E>(unexpect, std::move(error()));

            if constexpr (!std::is_void_v<U>)
                return expected<U, E>(std::invoke(std::forward<F>(f)));
            else
            {
                std::invoke(std::forward<F>(f));
                return expected<U, E>();
            }
        }

        template <class F>
            requires(std::is_move_constructible_v<E>)
        constexpr auto transform(F&& f) const&&
        {
            using U = std::remove_cvref_t<std::invoke_result_t<F>>;

            if (!_has_value)
                return expected<U, E>(unexpect, std::move(error()));

            if constexpr (!std::is_void_v<U>)
                return expected<U, E>(std::invoke(std::forward<F>(f)));
            else
            {
                std::invoke(std::forward<F>(f));
                return expected<U, E>();
            }
        }

        template <class F>
        constexpr auto transform_error(F&& f) &
        {
            using G = std::remove_cvref_t<std::invoke_result_t<F, decltype(error())>>;
            return _has_value ? expected<T, G>() : expected<T, G>(unexpect, std::invoke(std::forward<F>(f), error()));
        }

        template <class F>
        constexpr auto transform_error(F&& f) const&
        {
            using G = std::remove_cvref_t<std::invoke_result_t<F, decltype(error())>>;
            return _has_value ? expected<T, G>() : expected<T, G>(unexpect, std::invoke(std::forward<F>(f), error()));
        }

        template <class F>
        constexpr auto transform_error(F&& f) &&
        {
            using G = std::remove_cvref_t<std::invoke_result_t<F, decltype(std::move(error()))>>;
            return _has_value ? expected<T, G>() : expected<T, G>(unexpect, std::invoke(std::forward<F>(f), std::move(error())));
        }

        template <class F>
        constexpr auto transform_error(F&& f) const&&
        {
            using G = std::remove_cvref_t<std::invoke_result_t<F, decltype(std::move(error()))>>;
            return _has_value ? expected<T, G>() : expected<T, G>(unexpect, std::invoke(std::forward<F>(f), std::move(error())));
        }

        // Equality operators
        template <class T2, class E2>
            requires(std::is_void_v<T2>)
        friend constexpr bool operator==(const expected& x, const expected<T2, E2>& y)
        {
            if (x.has_value())
                return y.has_value();
            else
                return !y.has_value() && static_cast<bool>(x.error() == y.error());
        }

        template <class E2>
        friend constexpr bool operator==(const expected& x, const unexpected<E2>& e)
        {
            return !x.has_value() && static_cast<bool>(x.error() == e.value());
        }

#if defined(__GNUC__) && __GNUC__ < 10 && !defined(__clang__)
        template <class T2, class E2>
            requires(std::is_void_v<T2>)
        friend constexpr bool operator!=(const expected& x, const expected<T2, E2>& y)
        {
            return !(x == y);
        }

        template <class E2>
        friend constexpr bool operator!=(const expected& x, const unexpected<E2>& e)
        {
            return !(x == e);
        }
#endif

    private:
        constexpr void construct_value() { _has_value = true; }

        template <class... Args>
        constexpr void construct_error(Args&&... args)
        {
            detail::construct_at(std::addressof(_error), std::forward<Args>(args)...);
            _has_value = false;
        }

        union
        {
            E _error;
        };
        bool _has_value;
    };

} // namespace mtl
