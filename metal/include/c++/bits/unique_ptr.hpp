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

#pragma once

#include <type_traits>
#include <utility>

namespace std
{
    template <class T>
    class unique_ptr
    {
    public:
        using pointer = T*;

        constexpr unique_ptr() noexcept : _p{} {}

        constexpr unique_ptr(std::nullptr_t) noexcept : _p{} {}

        explicit constexpr unique_ptr(pointer p) noexcept : _p(p) {}

        constexpr unique_ptr(unique_ptr&& u) noexcept : _p(u.release()) {}

        template <class U>
        constexpr unique_ptr(unique_ptr<U>&& u) noexcept : _p(u.release())
        {}

        template <class U>
        explicit unique_ptr(U p) noexcept : _p(p)
        {}

// Older versions of GCC / mingw won't allow constexpr on destructors
#if defined(__clang__) || !(defined(__GNUC__) && __GNUC__ < 10)
        constexpr
#endif
            ~unique_ptr()
        {
            reset();
        }

        constexpr unique_ptr& operator=(unique_ptr&& r) noexcept
        {
            reset(r.release());
            return *this;
        }

        template <class U>
        constexpr unique_ptr& operator=(unique_ptr<U>&& r) noexcept
        {
            reset(r.release());
            return *this;
        }

        constexpr unique_ptr& operator=(std::nullptr_t) noexcept
        {
            reset();
            return *this;
        }

        constexpr pointer release() noexcept
        {
            pointer p = _p;
            _p = nullptr;
            return p;
        }

        constexpr void reset(pointer ptr = pointer()) noexcept
        {
            pointer old_ptr = _p;
            _p = ptr;
            if (old_ptr)
                delete old_ptr;
        }

        void swap(unique_ptr& other) noexcept
        {
            using std::swap;
            swap(_p, other._p);
        }

        constexpr pointer get() const noexcept { return _p; }

        constexpr explicit operator bool() const noexcept { return _p != nullptr; }

        constexpr typename std::add_lvalue_reference<T>::type operator*() const noexcept(noexcept(*std::declval<pointer>()))
        {
            return *get();
        }

        constexpr pointer operator->() const noexcept { return get(); }

    private:
        T* _p;
    };

    template <class T>
    constexpr void swap(std::unique_ptr<T>& lhs, std::unique_ptr<T>& rhs) noexcept
    {
        lhs.swap(rhs);
    }

    template <class T>
    class unique_ptr<T[]>;

    template <class T, class... Args>
    requires(!std::is_array_v<T>) unique_ptr<T> make_unique(Args&&... args)
    {
        return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
    }
} // namespace std
