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

#include "unique_ptr.hpp"
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
    template <class T>
    class shared_ptr;

    template <class T>
    class weak_ptr;

    namespace details
    {
        class RefCount
        {
        public:
            // No implicit copy / move
            RefCount(const RefCount&) = delete;
            RefCount& operator=(const RefCount&) = delete;

            void IncRef() noexcept { ++_count; }

            bool IncRefNotZero() noexcept
            {
                if (_count)
                {
                    ++_count;
                    return true;
                }

                return false;
            }

            void IncWeakRef() noexcept { ++_weak; }

            void DecRef() noexcept
            {
                if (--_count == 0)
                {
                    DestroyObject();
                    DecWeakRef();
                }
            }

            void DecWeakRef() noexcept
            {
                if (--_weak == 0)
                {
                    delete this;
                }
            }

            int use_count() const noexcept { return _count; }

        protected:
            virtual ~RefCount() noexcept {}

            constexpr RefCount() noexcept = default;

        private:
            virtual void DestroyObject() noexcept = 0;

            int _count{1};
            int _weak{1};
        };

        template <class T>
        class RefCountWithPointer : public RefCount
        {
        public:
            explicit RefCountWithPointer(T* p) : object{p} {}

        private:
            void DestroyObject() noexcept override { delete object; }

            T* object;
        };

        template <class T>
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
            void DestroyObject() noexcept override { object.~T(); }
        };

        template <class T>
        class _base_ptr
        {
        public:
            using element_type = std::remove_extent_t<T>;

            _base_ptr(const _base_ptr&) = delete;
            _base_ptr& operator=(const _base_ptr&) = delete;

            int use_count() const noexcept { return _rc ? _rc->use_count() : 0; }

        protected:
            constexpr _base_ptr() noexcept = default;
            ~_base_ptr() = default;

            template <typename U>
            friend class _base_ptr;

            friend class weak_ptr<T>;

            template <class U>
            void _ConstructFromRefCount(U* u, details::RefCount* rc) noexcept
            {
                _p = u;
                _rc = rc;
            }

            template <typename U>
            void _ConstructFromWeak(const weak_ptr<U>& rhs) noexcept
            {
                if (rhs._rc && rhs._rc->IncRefNotZero())
                {
                    _p = rhs._p;
                    _rc = rhs._rc;
                }
            }

            template <typename U>
            void _ConstructWeak(const _base_ptr<U>& rhs) noexcept
            {
                if (rhs._rc)
                {
                    _p = rhs._p;
                    _rc = rhs._rc;
                    _rc->IncWeakRef();
                }
            }

            template <typename U>
            void _CopyConstruct(const shared_ptr<U>& rhs) noexcept
            {
                rhs._IncRef();

                _p = rhs._p;
                _rc = rhs._rc;
            }

            template <typename U>
            void _MoveConstruct(_base_ptr<U>&& rhs) noexcept
            {
                _p = rhs._p;
                _rc = rhs._rc;

                rhs._p = nullptr;
                rhs._rc = nullptr;
            }

            void _IncRef() const noexcept
            {
                if (_rc)
                    _rc->IncRef();
            }

            void _IncWeakRef() const noexcept
            {
                if (_rc)
                    _rc->IncWeakRef();
            }

            void _DecRef() const noexcept
            {
                if (_rc)
                    _rc->DecRef();
            }

            void _DecWeakRef() const noexcept
            {
                if (_rc)
                    _rc->DecWeakRef();
            }

            void swap(_base_ptr& rhs) noexcept
            {
                std::swap(_p, rhs._p);
                std::swap(_rc, rhs._rc);
            }

            T* _p{nullptr};
            details::RefCount* _rc{nullptr};
        };

    } // namespace details

    template <class T>
    class shared_ptr : public details::_base_ptr<T>
    {
        using _base_ptr = details::_base_ptr<T>;

    public:
        using weak_type = _STD::weak_ptr<T>;

        constexpr shared_ptr() noexcept = default;
        constexpr shared_ptr(std::nullptr_t) noexcept {}

        template <class U>
        explicit shared_ptr(U* u)
        {
            _base_ptr::_ConstructFromRefCount(u, new details::RefCountWithPointer<U>(u));
        }

        template <class U>
        explicit shared_ptr(details::RefCountWithObject<U>* rc)
        {
            _base_ptr::_ConstructFromRefCount(&rc->object, rc);
        }

        shared_ptr(const shared_ptr& rhs) noexcept { _base_ptr::_CopyConstruct(rhs); }

        template <class U>
        shared_ptr(const shared_ptr<U>& rhs) noexcept
        {
            _base_ptr::_CopyConstruct(rhs);
        }

        shared_ptr(shared_ptr&& rhs) noexcept { _base_ptr::_MoveConstruct(std::move(rhs)); }

        template <class U>
        shared_ptr(shared_ptr<U>&& rhs) noexcept
        {
            _base_ptr::_MoveConstruct(std::move(rhs));
        }

        ~shared_ptr() { _base_ptr::_DecRef(); }

        shared_ptr& operator=(const shared_ptr& rhs) noexcept
        {
            shared_ptr(rhs).swap(*this);
            return *this;
        }

        template <class U>
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

        template <class U>
        shared_ptr& operator=(shared_ptr<U>&& rhs) noexcept
        {
            shared_ptr(std::move(rhs)).swap(*this);
            return *this;
        }

        void reset() noexcept { shared_ptr().swap(*this); }

        template <class U>
        void reset(U* p)
        {
            shared_ptr(p).swap(*this);
        }

        void swap(shared_ptr& rhs) noexcept { _base_ptr::swap(rhs); }

        T* get() const noexcept { return _base_ptr::_p; }
        T* operator->() const noexcept { return get(); }
        T& operator*() const noexcept { return *get(); }

        explicit operator bool() const noexcept { return get() != nullptr; }
    };

    template <class T, class... Args>
        requires(!std::is_array_v<T>)
    shared_ptr<T> make_shared(Args&&... args)
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

    template <class T>
    void swap(shared_ptr<T>& lhs, shared_ptr<T>& rhs) noexcept
    {
        lhs.swap(rhs);
    }

    template <class T>
    class weak_ptr : public details::_base_ptr<T>
    {
        using _base_ptr = details::_base_ptr<T>;

    public:
        constexpr weak_ptr() noexcept = default;

        weak_ptr(const weak_ptr<T>& rhs) noexcept { _base_ptr::_ConstructWeak(rhs); }

        template <typename U>
        weak_ptr(const shared_ptr<U>& rhs) noexcept
        {
            _base_ptr::_ConstructWeak(rhs);
        }

        weak_ptr(weak_ptr<T>&& rhs) noexcept { _base_ptr::_MoveConstruct(std::move(rhs)); }

        ~weak_ptr() { _base_ptr::_DecWeakRef(); }

        weak_ptr& operator=(const weak_ptr& rhs) noexcept
        {
            weak_ptr(rhs).swap(*this);
            return *this;
        }

        template <class U>
        weak_ptr& operator=(const weak_ptr<U>& rhs) noexcept
        {
            weak_ptr(rhs).swap(*this);
            return *this;
        }

        weak_ptr& operator=(weak_ptr&& rhs) noexcept
        {
            weak_ptr(std::move(rhs)).swap(*this);
            return *this;
        }

        template <class U>
        weak_ptr& operator=(weak_ptr<U>&& rhs) noexcept
        {
            weak_ptr(std::move(rhs)).swap(*this);
            return *this;
        }

        bool expired() const noexcept { return _base_ptr::use_count() == 0; }

        shared_ptr<T> lock() const noexcept
        {
            shared_ptr<T> x;
            x._ConstructFromWeak(*this);
            return x;
        }

        void reset() noexcept { weak_ptr{}.swap(*this); }

        void swap(weak_ptr& rhs) noexcept { _base_ptr::swap(rhs); }

    private:
    };

    template <class T>
    weak_ptr(shared_ptr<T>) -> weak_ptr<T>;

    template <class T>
    void swap(weak_ptr<T>& lhs, weak_ptr<T>& rhs) noexcept
    {
        lhs.swap(rhs);
    }

#if !UNITTEST
    // Unimplemented specializations
    template <typename T>
    struct atomic<shared_ptr<T>>;

    template <typename T>
    struct atomic<weak_ptr<T>>;
#endif

} // namespace _STD
