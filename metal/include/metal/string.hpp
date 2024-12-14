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

#include <algorithm>
#include <malloc.h>
#include <metal/helpers.hpp>
#include <metal/string_view.hpp>

namespace mtl
{
    template <typename T>
    constexpr int _strncmp(const T* a, const T* b, size_t count)
    {
        if (!count)
            return 0;
        else if (*a == *b)
            return _strncmp(++a, ++b, count - 1);
        else
            return *(unsigned char*)a - *(unsigned char*)b;
    }

    template <>
    constexpr int _strncmp(const char* a, const char* b, size_t count)
    {
        return __builtin_memcmp(a, b, count);
    }

#pragma GCC diagnostic push
#if !defined(__clang__)
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif

    template <typename T>
    class basic_string
    {
    public:
        using iterator = T*;
        using const_iterator = const T*;
        using reference = T&;
        using const_reference = const T&;
        using value_type = T;
        using size_type = std::size_t;

    private:
        struct _LargeString
        {
            T* data;
            size_type length;
            size_type capacity;
        };
        static_assert(sizeof(_LargeString) == 3 * sizeof(void*));

        struct _SmallString
        {
            T data[sizeof(_LargeString) / sizeof(T) - 1];
            T length; // kMaxSmallLength - length
        };
        static_assert(sizeof(_SmallString) == 3 * sizeof(void*));

        static_assert(sizeof(_SmallString) == sizeof(_LargeString));

        static constexpr auto kMaxSmallLength = sizeof(_SmallString::data) / sizeof(T);

#if defined(__x86_64__) || defined(__aarch64__)
        // Little endian
        static constexpr auto kLargeFlag = size_type(1) << (sizeof(size_type) * 8 - 1);
        static constexpr auto kCapacityMask = ~kLargeFlag;
#else
        // Big endian
        static constexpr auto kLargeFlag = 1;
        static constexpr auto kCapacityMask = ~kLargeFlag;
#endif

    public:
        // Construction
        constexpr basic_string() { _init<T>(nullptr, 0); }

        constexpr basic_string(const T* string) { _init(string, _strlen(string)); }

        constexpr basic_string(const T* string, size_type length) { _init(string, length); }

        template <typename U>
        constexpr basic_string(const U* first, const U* last)
        {
            _init(first, last - first);
        }

        constexpr basic_string(const basic_string& other) { _init(other.data(), other.length()); }

        constexpr basic_string(basic_string&& other) noexcept { _move(std::move(other)); }

        // Destruction
        constexpr ~basic_string() { _destroy(); }

        // Assignment
        constexpr basic_string& operator=(basic_string&& other)
        {
            _destroy();
            _move(std::move(other));
            return *this;
        }

        constexpr basic_string& assign(const T* s, size_type length)
        {
            // TODO: we could be smarter and reuse existing heap memory... do we care?
            _destroy();
            _init(s, length);
            return *this;
        }

        // Accessors
        constexpr size_type capacity() const noexcept { return _isLarge() ? _large.capacity & kCapacityMask : kMaxSmallLength; }
        constexpr size_type length() const { return _isSmall() ? kMaxSmallLength - _small.length : _large.length; }
        constexpr size_type size() const { return length(); }

        constexpr iterator begin() noexcept { return data(); }
        constexpr const_iterator begin() const noexcept { return data(); }
        constexpr const_iterator cbegin() const noexcept { return data(); }
        constexpr iterator end() noexcept { return data() + size(); }
        constexpr const_iterator end() const noexcept { return data() + size(); }
        constexpr const_iterator cend() const noexcept { return data() + size(); }

        // Conversion
        constexpr const T* data() const noexcept { return _isSmall() ? _small.data : _large.data; }
        constexpr const T* c_str() const noexcept { return data(); }
        constexpr operator mtl::basic_string_view<T>() const noexcept { return mtl::basic_string_view<T>(data(), length()); }

    private:
        template <typename U>
        constexpr void _init(const U* string, size_type length)
        {
            if (length <= kMaxSmallLength)
            {
                std::copy(string, string + length, _small.data);
                if (length != kMaxSmallLength)
                {
                    _small.data[length] = 0;
                    _small.length = kMaxSmallLength - length;
                }
                else
                {
                    _small.length = 0;
                }
            }
            else
            {
                _large.data = static_cast<T*>(::malloc(sizeof(T) * (length + 1)));
                _large.length = length;
                _large.capacity = (::malloc_usable_size(_large.data) / sizeof(T) - 1) | kLargeFlag;
                std::copy(string, string + length, _large.data);
                _large.data[length] = 0;
            }
        }

        constexpr void _move(basic_string&& other)
        {
            if (other._isSmall())
            {
                _small = other._small;
            }
            else
            {
                _large = other._large;
                other._init<T>(nullptr, 0);
            }
        }

        constexpr void _destroy()
        {
            if (_isLarge())
                ::free(_large.data);
        }

        union
        {
            _SmallString _small; // Small string optimization
            _LargeString _large; // String data on the heap
        };

        constexpr bool _isSmall() const { return !_isLarge(); }
        constexpr bool _isLarge() const { return _large.capacity & kLargeFlag; }
    };

    template <typename T>
    constexpr bool operator==(const mtl::basic_string<T>& lhs, const mtl::basic_string<T>& rhs) noexcept
    {
        if (lhs.size() != rhs.size())
            return false;
        return 0 == _strncmp(lhs.data(), rhs.data(), lhs.size());
    }

    template <typename T>
    constexpr bool operator==(const mtl::basic_string<T>& lhs, const T* rhs) noexcept
    {
        if (lhs.size() != _strlen(rhs))
            return false;
        return 0 == _strncmp(lhs.data(), rhs, lhs.size());
    }

    using string = basic_string<char>;
    using wstring = basic_string<wchar_t>;
    using u8string = basic_string<char8_t>;
    using u16string = basic_string<char16_t>;
    using u32string = basic_string<char32_t>;

#if defined(__clang__)
#pragma GCC diagnostic ignored "-Wuser-defined-literals"
#else
#pragma GCC diagnostic ignored "-Wliteral-suffix"
#endif

    namespace literals
    {
        constexpr mtl::string operator""s(const char* s, std::size_t length)
        {
            return mtl::string{s, length};
        }
        constexpr mtl::wstring operator""s(const wchar_t* s, std::size_t length)
        {
            return mtl::wstring{s, length};
        }
        constexpr mtl::u8string operator""s(const char8_t* s, std::size_t length)
        {
            return mtl::u8string{s, length};
        }
        constexpr mtl::u16string operator""s(const char16_t* s, std::size_t length)
        {
            return mtl::u16string{s, length};
        }
        constexpr mtl::u32string operator""s(const char32_t* s, std::size_t length)
        {
            return mtl::u32string{s, length};
        }

    } // namespace literals

#pragma GCC diagnostic pop

} // namespace mtl
