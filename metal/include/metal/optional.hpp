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
#include <initializer_list>
#include <metal/type_traits.hpp>
#include <utility>

namespace mtl
{
    struct nullopt_t
    {
        constexpr explicit nullopt_t(int) {}
    };

    inline constexpr nullopt_t nullopt{0};

    template <class T>
    class optional
    {
    public:
        using value_type = T;

        // Constructors
        constexpr optional() noexcept = default;
        constexpr optional(mtl::nullopt_t) noexcept {}

        constexpr optional(const optional& other)
            requires(std::is_copy_constructible_v<T> && !std::is_trivially_constructible_v<T>)
        {
            if (other)
                _construct(*other);
        }

        constexpr optional(const optional& other)
            requires(std::is_copy_constructible_v<T> && std::is_trivially_constructible_v<T>)
        = default;

        constexpr optional(const optional& other)
            requires(!std::is_copy_constructible_v<T>)
        = delete;

#if defined(__clang__)
        constexpr optional(const optional&& other) noexcept(std::is_nothrow_move_constructible_v<T>)
            requires(std::is_move_constructible_v<T> && !std::is_trivially_move_constructible_v<T>)
        {
            if (other)
                _construct(std::move(*other));
        }

        constexpr optional(const optional&& other) noexcept(std::is_nothrow_move_constructible_v<T>)
            requires(std::is_move_constructible_v<T> && std::is_trivially_move_constructible_v<T>)
        = default;

        constexpr optional(const optional&& other) noexcept(std::is_nothrow_move_constructible_v<T>)
            requires(!std::is_move_constructible_v<T>)
        = delete;
#else
        constexpr optional(const optional&& other) noexcept(std::is_nothrow_move_constructible_v<T>)
            requires(std::is_move_constructible_v<T>)
        {
            if (other)
                _construct(std::move(*other));
        }
#endif
        template <class U>
        explicit(std::is_convertible_v<const U&, T>) constexpr optional(const optional<U>& other)
            requires(std::is_constructible_v<T, const U&> &&
                     (!std::is_same_v<std::remove_cvref_t<T>, bool> && !std::is_constructible_v<T, mtl::optional<U>&> &&
                      !std::is_constructible_v<T, const mtl::optional<U>&> && !std::is_constructible_v<T, mtl::optional<U> &&> &&
                      !std::is_constructible_v<T, const mtl::optional<U> &&> && !std::is_convertible_v<mtl::optional<U>&, T> &&
                      !std::is_convertible_v<const mtl::optional<U>&, T> && !std::is_convertible_v<mtl::optional<U> &&, T> &&
                      !std::is_convertible_v<const mtl::optional<U> &&, T>))
        {
            if (other)
                _construct(*other);
        }

        template <class U>
        explicit(std::is_convertible_v<U&&, T>) constexpr optional(const optional<U>&& other)
            requires(std::is_constructible_v<T, U &&> &&
                     (!std::is_same_v<std::remove_cvref_t<T>, bool> && !std::is_constructible_v<T, mtl::optional<U>&> &&
                      !std::is_constructible_v<T, const mtl::optional<U>&> && !std::is_constructible_v<T, mtl::optional<U> &&> &&
                      !std::is_constructible_v<T, const mtl::optional<U> &&> && !std::is_convertible_v<mtl::optional<U>&, T> &&
                      !std::is_convertible_v<const mtl::optional<U>&, T> && !std::is_convertible_v<mtl::optional<U> &&, T> &&
                      !std::is_convertible_v<const mtl::optional<U> &&, T>))
        {
            if (other)
                _construct(std::move(*other));
        }

        template <class... Args>
        constexpr explicit optional(std::in_place_t, Args&&... args)
            requires(std::is_constructible_v<T, Args...>)
        {
            _construct(std::forward<Args>(args)...);
        }

        template <class U, class... Args>
        constexpr explicit optional(std::in_place_t, std::initializer_list<U> ilist, Args&&... args)
            requires(std::is_constructible_v<T, std::initializer_list<U>&, Args...>)
        {
            _construct(ilist, std::forward<Args>(args)...);
        }

        template <class U = T>
        explicit(std::is_convertible_v<U&&, T>) constexpr optional(U&& value)
            requires(std::is_constructible_v<T, U &&> &&
                     (!std::is_same_v<std::remove_cvref_t<U>, std::in_place_t> &&
                      !std::is_same_v<std::remove_cvref_t<U>, mtl::optional<T>>) &&
                     !(std::is_same_v<std::remove_cvref_t<T>, bool> &&
                       !mtl::is_specialization<std::remove_cvref_t<U>, mtl::optional>::value))
        {
            _construct(std::forward<U>(value));
        }

        // Destructor
        constexpr ~optional()
            requires(!std::is_trivially_destructible_v<T>)
        {
            _destroy();
        }

        constexpr ~optional()
            requires(std::is_trivially_destructible_v<T>)
        = default;

        // Assignment
        constexpr optional& operator=(mtl::nullopt_t) noexcept { _destroy(); }

        constexpr optional& operator=(const optional& other)
            requires(std::is_copy_constructible_v<T> && std::is_copy_assignable_v<T>)
        {
            _destroy();
            if (other)
                _construct(*other);
        }
        constexpr optional& operator=(const optional& other)
            requires(!std::is_copy_constructible_v<T> || !std::is_copy_assignable_v<T>)
        = delete;

        constexpr optional& operator=(optional&& other) noexcept(std::is_nothrow_move_assignable_v<T> &&
                                                                 std::is_nothrow_move_constructible_v<T>)
            requires(std::is_trivially_move_constructible_v<T> && std::is_trivially_move_assignable_v<T> &&
                     std::is_trivially_destructible_v<T>)
        = default;

        constexpr optional& operator=(optional&& other) noexcept(std::is_nothrow_move_assignable_v<T> &&
                                                                 std::is_nothrow_move_constructible_v<T>)
            requires(std::is_move_constructible_v<T> && std::is_move_assignable_v<T> &&
                     !(std::is_trivially_move_constructible_v<T> && std::is_trivially_move_assignable_v<T> &&
                       std::is_trivially_destructible_v<T>))
        {
            _destroy();
            if (other)
                _construct(std::move(*other));
        }

        template <class U = T>
        constexpr optional& operator=(U&& value)
            requires(!std::is_same_v<std::remove_cvref_t<U>, mtl::optional<T>> && std::is_constructible_v<T, U>,
                     std::is_assignable_v<T&, U> && (std::is_scalar_v<T> || !std::is_same_v<std::decay_t<U>, T>))
        {
            if (_has_value)
                _value = std::forward<U>(value);
            else
                _construct(std::forward<U>(value));
        }

        template <class U>
        constexpr optional& operator=(const optional<U>& other)
            requires(!std::is_constructible_v<T, mtl::optional<U>&> && !std::is_constructible_v<T, const mtl::optional<U>&> &&
                     !std::is_constructible_v<T, mtl::optional<U> &&> && !std::is_constructible_v<T, const mtl::optional<U> &&> &&
                     !std::is_convertible_v<mtl::optional<U>&, T> && !std::is_convertible_v<const mtl::optional<U>&, T> &&
                     !std::is_convertible_v<mtl::optional<U> &&, T> && !std::is_convertible_v<const mtl::optional<U> &&, T> &&
                     !std::is_assignable_v<T&, mtl::optional<U>&> && !std::is_assignable_v<T&, const mtl::optional<U>&> &&
                     !std::is_assignable_v<T&, mtl::optional<U> &&> && !std::is_assignable_v<T&, const mtl::optional<U> &&> &&
                     std::is_constructible_v<T, const U&> && std::is_assignable_v<T&, const U&>

            )
        {
            if (other)
            {
                if (_has_value)
                    _value = std::forward<U>(*other);
                else
                    _construct(std::forward<U>(*other));
            }
            else
            {
                _destroy();
            }
        }

        template <class U>
        constexpr optional& operator=(optional<U>&& other)
            requires(!std::is_constructible_v<T, mtl::optional<U>&> && !std::is_constructible_v<T, const mtl::optional<U>&> &&
                     !std::is_constructible_v<T, mtl::optional<U> &&> && !std::is_constructible_v<T, const mtl::optional<U> &&> &&
                     !std::is_convertible_v<mtl::optional<U>&, T> && !std::is_convertible_v<const mtl::optional<U>&, T> &&
                     !std::is_convertible_v<mtl::optional<U> &&, T> && !std::is_convertible_v<const mtl::optional<U> &&, T> &&
                     !std::is_assignable_v<T&, mtl::optional<U>&> && !std::is_assignable_v<T&, const mtl::optional<U>&> &&
                     !std::is_assignable_v<T&, mtl::optional<U> &&> && !std::is_assignable_v<T&, const mtl::optional<U> &&> &&
                     std::is_constructible_v<T, U> && std::is_assignable_v<T&, U>

            )
        {
            if (other)
            {
                if (_has_value)
                    _value = std::forward<U>(std::move(*other));
                else
                    _construct(std::forward<U>(std::move(*other)));
            }
            else
            {
                _destroy();
            }
        }

        // Observers
        constexpr const T* operator->() const noexcept { return &_value; }
        constexpr T* operator->() noexcept { return &_value; }
        constexpr const T& operator*() const& noexcept { return _value; }
        constexpr T& operator*() & noexcept { return _value; }
        constexpr const T&& operator*() const&& noexcept { return std::move(_value); }
        constexpr T&& operator*() && noexcept { return std::move(_value); }

        constexpr explicit operator bool() const noexcept { return _has_value; }
        constexpr bool has_value() const noexcept { return _has_value; }

        constexpr T& value() &
        {
            assert(_has_value);
            return _value;
        }

        constexpr const T& value() const&
        {
            assert(_has_value);
            return _value;
        }

        constexpr T&& value() &&
        {
            assert(_has_value);
            return std::move(_value);
        }

        constexpr const T&& value() const&&
        {
            assert(_has_value);
            return std::move(_value);
        }

        // Monadic operations
        template <class F>
        constexpr auto and_then(F&& f) &
        {
            if (*this)
                return std::invoke(std::forward<F>(f), **this);
            else
                return std::remove_cvref_t<std::invoke_result_t<F, T&>>{};
        }

        template <class F>
        constexpr auto and_then(F&& f) const&
        {
            if (*this)
                return std::invoke(std::forward<F>(f), **this);
            else
                return std::remove_cvref_t<std::invoke_result_t<F, const T&>>{};
        }

        template <class F>
        constexpr auto and_then(F&& f) &&
        {
            if (*this)
                return std::invoke(std::forward<F>(f), std::move(**this));
            else
                return std::remove_cvref_t<std::invoke_result_t<F, T>>{};
        }

        template <class F>
        constexpr auto and_then(F&& f) const&&
        {
            if (*this)
                return std::invoke(std::forward<F>(f), std::move(**this));
            else
                return std::remove_cvref_t<std::invoke_result_t<F, const T>>{};
        }

        template <class F>
        constexpr auto transform(F&& f) &
        {
            using U = std::remove_cv_t<std::invoke_result_t<F, T&>>;
            if (*this)
                return mtl::optional<U>(std::invoke(std::forward<F>(f), **this));
            else
                return mtl::optional<U>();
        }

        template <class F>
        constexpr auto transform(F&& f) const&
        {
            using U = std::remove_cv_t<std::invoke_result_t<F, const T&>>;
            if (*this)
                return mtl::optional<U>(std::invoke(std::forward<F>(f), **this));
            else
                return mtl::optional<U>();
        }

        template <class F>
        constexpr auto transform(F&& f) &&
        {
            using U = std::remove_cv_t<std::invoke_result_t<F, T>>;
            if (*this)
                return mtl::optional<U>(std::invoke(std::forward<F>(f), std::move(**this)));
            else
                return mtl::optional<U>();
        }

        template <class F>
        constexpr auto transform(F&& f) const&&
        {
            using U = std::remove_cv_t<std::invoke_result_t<F, const T>>;
            if (*this)
                return mtl::optional<U>(std::invoke(std::forward<F>(f), std::move(**this)));
            else
                return mtl::optional<U>();
        }

        template <class F>
        constexpr optional or_else(F&& f) const&
            requires(std::is_copy_constructible_v<T> && std::is_invocable_v<F>)
        {
            return *this ? *this : std::forward<F>(f)();
        }

        template <class F>
            constexpr optional or_else(F&& f) &&
            requires(std::is_move_constructible_v<T>&& std::is_invocable_v<F>) {
                return *this ? std::move(*this) : std::forward<F>(f)();
            }

            // Modifiers
            constexpr void swap(optional& other) noexcept(std::is_nothrow_move_constructible_v<T> && std::is_nothrow_swappable_v<T>)
        {
            if (*this)
            {
                if (other)
                {
                    using std::swap;
                    std::swap(**this, *other);
                }
                else
                {
                    other._construct(std::move(**this));
                    _destroy();
                }
            }
            else if (other)
            {
                _construct(std::move(*other));
                other._destroy();
            }
        }

        constexpr void reset() noexcept { _destroy(); }

        template <class... Args>
        constexpr T& emplace(Args&&... args)
        {
            _destroy();
            _construct(std::forward<Args>(args)...);
        }

        template <class U, class... Args>
        constexpr T& emplace(std::initializer_list<U> ilist, Args&&... args)
            requires(std::is_constructible_v<T, std::initializer_list<U>&, Args && ...>)
        {
            _destroy();
            _construct(ilist, std::forward<Args>(args)...);
        }

    private:
        template <typename... Args>
        constexpr void _construct(Args&&... args)
        {
            ::new (const_cast<void*>(static_cast<const volatile void*>(&_value))) T(std::forward<Args>(args)...);
            _has_value = true;
        }

        constexpr void _destroy()
        {
            if (_has_value)
            {
                _value.~T();
                _has_value = false;
            }
        }

        bool _has_value{false};
        union
        {
            T _value;
        };
    };

    // Comparison operators
    template <class T, class U>
    constexpr bool operator==(const optional<T>& lhs, const optional<U>& rhs)
    {
        if (lhs && rhs)
            return *lhs == *rhs;
        if (!lhs && !rhs)
            return true;
        return false;
    }

    template <class T, class U>
    constexpr bool operator!=(const optional<T>& lhs, const optional<U>& rhs)
    {
        return !(lhs == rhs);
    }

    template <class T, class U>
    constexpr bool operator<(const optional<T>& lhs, const optional<U>& rhs)
    {
        if (lhs && rhs)
            return *lhs < *rhs;
        if (!lhs && rhs)
            return true;
        return false;
    }

    template <class T, class U>
    constexpr bool operator<=(const optional<T>& lhs, const optional<U>& rhs)
    {
        return !(rhs < lhs);
    }

    template <class T, class U>
    constexpr bool operator>(const optional<T>& lhs, const optional<U>& rhs)
    {
        return rhs < lhs;
    }

    template <class T, class U>
    constexpr bool operator>=(const optional<T>& lhs, const optional<U>& rhs)
    {
        return !(lhs < rhs);
    }

    template <class T>
    constexpr bool operator==(const optional<T>& opt, mtl::nullopt_t) noexcept
    {
        return !opt;
    }

    template <class T, class U>
    constexpr bool operator==(const optional<T>& opt, const U& value)
    {
        return opt && *opt == value;
    }

    template <class T, class U>
    constexpr bool operator==(const T& value, const optional<U>& opt)
    {
        return opt == value;
    }

    template <class T, class U>
    constexpr bool operator!=(const optional<T>& opt, const U& value)
    {
        return opt != value;
    }

    template <class T, class U>
    constexpr bool operator!=(const T& value, const optional<U>& opt)
    {
        return value != opt;
    }

    template <class T, class U>
    constexpr bool operator<(const optional<T>& opt, const U& value)
    {
        return opt && *opt < value;
    }

    template <class T, class U>
    constexpr bool operator<(const T& value, const optional<U>& opt)
    {
        return opt ? value < *opt : true;
    }

    template <class T, class U>
    constexpr bool operator<=(const optional<T>& opt, const U& value)
    {
        return !(value < opt);
    }

    template <class T, class U>
    constexpr bool operator<=(const T& value, const optional<U>& opt)
    {
        return !(opt < value);
    }

    template <class T, class U>
    constexpr bool operator>(const optional<T>& opt, const U& value)
    {
        return value < opt;
    }

    template <class T, class U>
    constexpr bool operator>(const T& value, const optional<U>& opt)
    {
        return opt < value;
    }

    template <class T, class U>
    constexpr bool operator>=(const optional<T>& opt, const U& value)
    {
        return !(value < opt);
    }

    template <class T, class U>
    constexpr bool operator>=(const T& value, const optional<U>& opt)
    {
        return !(opt < value);
    }

    // make_optional
    template <class T>
    constexpr mtl::optional<std::decay_t<T>> make_optional(T&& value)
    {
        return mtl::optional<std::decay_t<T>>(std::forward<T>(value));
    }

    template <class T, class... Args>
    constexpr mtl::optional<T> make_optional(Args&&... args)
    {
        return mtl::optional<T>(std::in_place, std::forward<Args>(args)...);
    }

    template <class T, class U, class... Args>
    constexpr mtl::optional<T> make_optional(std::initializer_list<U> il, Args&&... args)
    {
        return mtl::optional<T>(std::in_place, il, std::forward<Args>(args)...);
    }

} // namespace mtl
