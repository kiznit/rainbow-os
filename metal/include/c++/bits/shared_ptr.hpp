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
    namespace details
    {
        struct RefCounter
        {
            RefCounter() : count(1), weak(0) {}
            virtual ~RefCounter() {}

            int count; // TODO: need to be using std::atomic<>
            int weak;  // TODO: need to be using std::atomic<>
        };

        template <typename U>
        struct RefCounterPointer : public RefCounter
        {
            explicit RefCounterPointer(U* u) : p(u) {}
            ~RefCounterPointer() override { delete p; }

            U* p;
        };

        template <typename U>
        struct RefCounterObject : public RefCounter
        {
            template <class... Args>
            RefCounterObject(Args&&... args) : object(std::forward<Args>(args)...)
            {}

            U object;
        };

    } // namespace details

    template <class T>
    class weak_ptr;

    template <class T>
    class shared_ptr
    {
    public:
        using element_type = std::remove_extent_t<T>;
        using weak_type = std::weak_ptr<T>;

        constexpr shared_ptr() noexcept : _p(nullptr), _rc(new details::RefCounterPointer<T>(nullptr)) {}

        constexpr shared_ptr(std::nullptr_t) noexcept : shared_ptr() {}

        template <class U>
        explicit shared_ptr(U* u) : _p(u), _rc(new details::RefCounterPointer<U>(u))
        {}

        template <typename U>
        explicit shared_ptr(details::RefCounterObject<U>* rc) : _p(&rc->object), _rc(rc)
        {}

        shared_ptr(const shared_ptr& s) : _p(s._p), _rc(s._rc) { inc(); }

        template <class U>
        shared_ptr(const shared_ptr<U>& s) : _p(s._p), _rc(s._rc)
        {
            inc();
        }

        shared_ptr& operator=(const shared_ptr& s)
        {
            if (this != &s)
            {
                // TODO: ensure we have no race conditions here when we switch to std::atomic.
                // We don't want "s" to be destroyed while we are trying to copy it.
                dec();
                _p = s._p;
                _rc = s._rc;
                inc();
            }
            return *this;
        }

        ~shared_ptr() { dec(); }

        void reset() noexcept { *this = std::shared_ptr<T>(nullptr); }

        template <class U>
        void reset(U* p)
        {
            *this = std::shared_ptr<U>(p);
        }

        T* get() const { return _p; }
        T* operator->() const { return _p; }
        T& operator*() const { return *_p; }

        explicit operator bool() const noexcept { return _p != nullptr; }

    private:
        void inc() { ++_rc->count; }

        void dec()
        {
            if (--_rc->count == 0)
                delete _rc;
        }

        T* _p;
        details::RefCounter* _rc;
    };

    template <class T, class... Args>
    requires(!std::is_array_v<T>) shared_ptr<T> make_shared(Args&&... args)
    {
        return shared_ptr<T>(new details::RefCounterObject<T>(std::forward<Args>(args)...));
    }

    template <class T, class U>
    bool operator==(const std::shared_ptr<T>& lhs, const std::shared_ptr<U>& rhs) noexcept
    {
        return lhs.get() == rhs.get();
    }

    template <class T>
    bool operator==(const std::shared_ptr<T>& lhs, std::nullptr_t) noexcept
    {
        return !lhs;
    }

} // namespace std