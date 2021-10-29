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

#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>

namespace metal
{
    // static_vector is not part of the C++ standard at time of writing.
    // http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0843r2.html

    template <typename T, size_t N>
    class static_vector
    {
    public:
        using value_type = T;
        using pointer = T*;
        using const_pointer = const T*;
        using reference = value_type&;
        using const_reference = const value_type&;
        using size_type = size_t;
        using difference_type = std::make_signed_t<size_type>;
        using iterator = T*;
        using const_iterator = const T*;

        constexpr static_vector() { _size = 0; }
        ~static_vector() { clear(); }

        constexpr iterator begin() { return _data; }
        constexpr iterator end() { return _data + _size; }
        constexpr const_iterator begin() const { return _data; }
        constexpr const_iterator end() const { return _data + _size; }
        constexpr const_iterator cbegin() const { return _data; }
        constexpr const_iterator cend() const { return _data + _size; }

        constexpr reference operator[](size_type i) { return _data[i]; }
        constexpr const_reference operator[](size_type i) const { return _data[i]; }
        constexpr reference front() { return _data[0]; }
        constexpr const_reference front() const { return _data[0]; }
        constexpr reference back() { return _data[_size - 1]; }
        constexpr const_reference back() const { return _data[_size - 1]; }

        constexpr T* data() { return _data; }
        constexpr const T* data() const { return _data; }
        constexpr bool empty() const { return _size == 0; }
        constexpr size_type size() const { return _size; }

        static constexpr size_type max_size() { return N; }
        static constexpr size_type capacity() { return N; }

        // Will not throw
        template <typename... Args>
        constexpr void emplace_back(Args&&... args)
        {
            if (_size < N)
            {
                new (&_data[_size]) T(std::forward<Args>(args)...);
                ++_size;
            }
        }

        // Will not throw
        constexpr void push_back(const value_type& value) { emplace_back(value); }
        constexpr void push_back(value_type&& value) { emplace_back(std::move(value)); }

        constexpr void clear()
        {
            for (auto& value : *this)
            {
                value.~T();
            }
            _size = 0;
        }

    private:
        T _data[N];
        size_type _size;
    };

} // namespace metal
