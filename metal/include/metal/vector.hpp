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
#include <cstdlib>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <new>

namespace mtl
{
    template <typename T>
    class vector
    {
    public:
        using iterator = T*;
        using const_iterator = const T*;
        using reference = T&;
        using const_reference = const T&;
        using size_type = std::size_t;
        using value_type = T;

        constexpr vector() : _first(0), _last(0), _max(0) {}
        constexpr vector(const vector& other) = delete; // not implemented (yet)
        vector(vector&& other) noexcept : vector() { swap(other); }

        template <class InputIt>
        constexpr vector(InputIt first, InputIt last) : vector()
        {
            reserve(std::distance(first, last));
            for (auto it = first; it != last; ++it)
                emplace_back(*it);
        }

        constexpr vector(std::initializer_list<T> init) : vector()
        {
            reserve(init.size());
            for (const auto& value : init)
                emplace_back(value);
        }

        constexpr ~vector() { clear(); }

        constexpr vector& operator=(const vector& other) = delete; // not implemented (yet)

        constexpr vector& operator=(vector&& other)
        {
            clear();
            swap(other);
            return *this;
        }

        constexpr reference operator[](size_type pos) { return _first[pos]; }
        constexpr const_reference operator[](size_type pos) const { return _first[pos]; }

        constexpr reference front() { return *_first; }
        constexpr const_reference front() const { return *_first; }
        constexpr reference back() { return *(_last - 1); }
        constexpr const_reference back() const { return *(_last - 1); }

        constexpr T* data() noexcept { return _first; }
        constexpr const T* data() const noexcept { return _first; }

        constexpr iterator begin() noexcept { return _first; }
        constexpr const_iterator begin() const noexcept { return _first; }
        constexpr iterator end() noexcept { return _last; }
        constexpr const_iterator end() const noexcept { return _last; }

        [[nodiscard]] constexpr bool empty() const noexcept { return _first == _last; }

        constexpr size_type size() const noexcept { return _last - _first; }

        constexpr void reserve(size_type newCapacity)
        {
            if (newCapacity > capacity())
            {
                const auto oldSize = size();

                // Allocate new memory
                T* newMemory = (T*)::malloc(newCapacity * sizeof(T));
                assert(newMemory); // TODO: we can't handle this very well in the kernel...

                // Move objects from old location ot the new one
                std::uninitialized_move(_first, _last, newMemory);

                // Destroy objects from old location
                for (auto p = _first; p != _last; ++p)
                {
                    p->T::~T();
                }

                // Free old memory
                ::free(_first);

                _first = newMemory;
                _last = newMemory + oldSize;
                _max = newMemory + newCapacity;
            }
        }

        constexpr size_type capacity() const noexcept { return _max - _first; }

        constexpr void clear() noexcept
        {
            for (auto p = _first; p != _last; ++p)
            {
                p->T::~T();
            }

            ::free(_first);
            _first = _last = _max = nullptr;
        }

        constexpr void push_back(const T& value) { emplace_back(value); }

        constexpr void push_back(T&& value) { emplace_back(std::move(value)); }

        template <class... Args>
        constexpr reference emplace_back(Args&&... args)
        {
            const auto oldSize = size();

            // Is the vector full?
            if (oldSize == capacity())
            {
                // Yes: allocate more memory
                reserve(oldSize ? oldSize * 2 : 16);
            }

            std::construct_at(_last, std::forward<Args>(args)...);
            return *_last++;
        }

        constexpr void pop_back()
        {
            (_last - 1)->~T();
            --_last;
        }

        constexpr void resize(size_type newSize)
        {
            const auto oldSize = size();

            if (newSize > oldSize)
            {
                // Make sure there is enough room
                reserve(newSize);

                // Build new objects
                for (auto i = oldSize; i != newSize; ++i)
                {
                    new (_first + i) T();
                }

                _last = _first + newSize;
            }
            else
            {
                // Destroy objects over the new size
                for (auto i = newSize; i != oldSize; ++i)
                {
                    (_first + i)->~T();
                }

                _last = _first + newSize;
            }
        }

        constexpr void swap(vector& other)
        {
            std::swap(_first, other._first);
            std::swap(_last, other._last);
            std::swap(_max, other._max);
        }

    private:
        T* _first; // begin()
        T* _last;  // end()
        T* _max;   // capacity()
    };

    template <class T>
    void swap(vector<T>& x, vector<T>& y) noexcept
    {
        x.swap(y);
    }
} // namespace mtl
