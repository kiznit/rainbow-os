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

#include <cstddef>
#include <cstring>

namespace mtl
{
    template <typename T>
    constexpr std::size_t _strlen(const T* string)
    {
        return *string ? 1 + _strlen(string + 1) : 0;
    }

    template <>
    constexpr std::size_t _strlen(const char* string)
    {
        return ::__builtin_strlen(string);
    }

    template <typename T>
    class basic_string_view
    {
    public:
        constexpr basic_string_view() : _string(nullptr), _length(0) {}
        constexpr basic_string_view(const basic_string_view&) = default;
        constexpr basic_string_view& operator=(const basic_string_view&) = default;
        constexpr basic_string_view(const T* s) : _string(s), _length(_strlen(s)) {}
        constexpr basic_string_view(std::nullptr_t) = delete;
        constexpr basic_string_view(const T* s, std::size_t length) : _string(s), _length(length) {}

        constexpr const T* begin() const { return _string; }
        constexpr const T* end() const { return _string + _length; }
        constexpr const T* cbegin() const { return _string; }
        constexpr const T* cend() const { return _string + _length; }
        constexpr std::size_t size() const { return _length; }

        constexpr const T* data() const { return _string; }
        constexpr std::size_t length() const { return _length; }

        constexpr const T& operator[](std::size_t pos) const { return _string[pos]; }

    private:
        const T* _string;
        std::size_t _length;
    };

    using string_view = basic_string_view<char>;
    using wstring_view = basic_string_view<wchar_t>;
    using u8string_view = basic_string_view<char8_t>;
    using u16string_view = basic_string_view<char16_t>;
    using u32string_view = basic_string_view<char32_t>;

    template <typename T>
    constexpr bool operator==(mtl::basic_string_view<T> lhs, mtl::basic_string_view<T> rhs) noexcept
    {
        return lhs.length() == rhs.length() && 0 == memcmp(lhs.data(), rhs.data(), lhs.length());
    }

    namespace literals
    {
#pragma GCC diagnostic push
#if defined(__clang__)
#pragma GCC diagnostic ignored "-Wuser-defined-literals"
#else
#pragma GCC diagnostic ignored "-Wliteral-suffix"
#endif

        constexpr mtl::string_view operator""sv(const char* s, std::size_t length) noexcept
        {
            return mtl::string_view{s, length};
        }
        constexpr mtl::wstring_view operator""sv(const wchar_t* s, std::size_t length) noexcept
        {
            return mtl::wstring_view{s, length};
        }
        constexpr mtl::u8string_view operator""sv(const char8_t* s, std::size_t length) noexcept
        {
            return mtl::u8string_view{s, length};
        }
        constexpr mtl::u16string_view operator""sv(const char16_t* s, std::size_t length) noexcept
        {
            return mtl::u16string_view{s, length};
        }
        constexpr mtl::u32string_view operator""sv(const char32_t* s, std::size_t length) noexcept
        {
            return mtl::u32string_view{s, length};
        }

#pragma GCC diagnostic pop
    } // namespace literals
} // namespace mtl
