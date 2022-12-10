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

#include <atomic>
#include <type_traits>
#include <utility>

#if UNITTEST
#define _STD std_test
#else
#define _STD std
#endif

namespace _STD
{
    namespace details
    {
        class RefCount
        {
        public:
            RefCount(const RefCount&) = delete;
            RefCount& operator=(const RefCount&) = delete;

            // TODO: can we use memory_order::relaxed here?
            void IncRef() noexcept { ++count; }

            // TODO: can we use memory_order::acq_rel here?
            void DecRef() noexcept
            {
                if (--count == 0)
                {
                    Destroy();
                    delete this;
                }
            }

            long use_count() const noexcept { return count.load(std::memory_order_relaxed); }

        protected:
            virtual ~RefCount() noexcept {}

            constexpr RefCount() noexcept = default;

        private:
            virtual void Destroy() noexcept = 0;

            std::atomic<int> count{1};
        };

        template <typename T>
        class RefCountWithPointer : public RefCount
        {
        public:
            explicit RefCountWithPointer(T* p) : object{p} {}

        private:
            void Destroy() noexcept override { delete object; }

            T* object;
        };

        template <typename T>
        class RefCountWithObject : public RefCount
        {
        public:
            template <class... Args>
            RefCountWithObject(Args&&... args) : object(std::forward<Args>(args)...)
            {
            }

            ~RefCountWithObject() override {}

            // Using an union so that ~RefCountWithObject() doesn't destroy the object
            union
            {
                T object;
            };

        private:
            void Destroy() noexcept override { object.~T(); }
        };

    } // namespace details

    template <class T>
    class weak_ptr;

    template <class T>
    class shared_ptr
    {
    public:
        using element_type = std::remove_extent_t<T>;
        using weak_type = _STD::weak_ptr<T>;

        constexpr shared_ptr() noexcept = default;
        constexpr shared_ptr(std::nullptr_t) noexcept {}

        template <class U>
        explicit shared_ptr(U* u) : _p(u), _rc(new details::RefCountWithPointer<U>(u))
        {
        }

        template <typename U>
        explicit shared_ptr(details::RefCountWithObject<U>* rc) : _p(&rc->object), _rc(rc)
        {
        }

        shared_ptr(const shared_ptr& rhs) noexcept
        {
            rhs._IncRef();

            _p = rhs._p;
            _rc = rhs._rc;
        }

        template <class U>
        shared_ptr(const shared_ptr<U>& rhs) noexcept
        {
            rhs._IncRef();

            _p = rhs._p;
            _rc = rhs._rc;
        }

        shared_ptr(shared_ptr&& rhs) noexcept
        {
            _p = rhs._p;
            _rc = rhs._rc;

            rhs._p = nullptr;
            rhs._rc = nullptr;
        }

        template <class U>
        shared_ptr(shared_ptr<U>&& rhs) noexcept
        {
            _p = rhs._p;
            _rc = rhs._rc;

            rhs._p = nullptr;
            rhs._rc = nullptr;
        }

        shared_ptr& operator=(const shared_ptr& rhs) noexcept
        {
            shared_ptr(rhs).swap(*this);
            return *this;
        }

        template <typename U>
        shared_ptr& operator=(const shared_ptr<U>& rhs) noexcept
        {
            shared_ptr(rhs).swap(*this);
            return *this;
        }

        shared_ptr& operator=(shared_ptr&& rhs) noexcept
        {
            shared_ptr(std::move(rhs)).swap(*this);
            return *this;
        }

        template <typename U>
        shared_ptr& operator=(shared_ptr<U>&& rhs) noexcept
        {
            shared_ptr(std::move(rhs)).swap(*this);
            return *this;
        }

        ~shared_ptr() { _DecRef(); }

        void reset() noexcept { shared_ptr().swap(*this); }

        template <class U>
        void reset(U* p)
        {
            shared_ptr(p).swap(*this);
        }

        void swap(shared_ptr& rhs) noexcept
        {
            std::swap(_p, rhs._p);
            std::swap(_rc, rhs._rc);
        }

        T* get() const noexcept { return _p; }
        T* operator->() const noexcept { return _p; }
        T& operator*() const noexcept { return *_p; }

        long use_count() const noexcept { return _rc ? _rc->use_count() : 0; }

        explicit operator bool() const noexcept { return _p != nullptr; }

        void _IncRef() const noexcept
        {
            if (_rc)
                _rc->IncRef();
        }

        void _DecRef() const noexcept
        {
            if (_rc)
                _rc->DecRef();
        }

        T* _p{nullptr};
        details::RefCount* _rc{nullptr};
    };

    template <class T, class... Args>
    requires(!std::is_array_v<T>) shared_ptr<T> make_shared(Args&&... args)
    {
        return shared_ptr<T>(new details::RefCountWithObject<T>(std::forward<Args>(args)...));
    }

    template <class T, class U>
    bool operator==(const _STD::shared_ptr<T>& lhs, const _STD::shared_ptr<U>& rhs) noexcept
    {
        return lhs.get() == rhs.get();
    }

    template <class T>
    bool operator==(const _STD::shared_ptr<T>& lhs, std::nullptr_t) noexcept
    {
        return !lhs;
    }

} // namespace _STD
