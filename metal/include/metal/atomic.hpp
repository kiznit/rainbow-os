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

#include <type_traits>

namespace mtl
{
    enum class memory_order : int
    {
        relaxed = __ATOMIC_RELAXED,
        consume = __ATOMIC_CONSUME,
        acquire = __ATOMIC_ACQUIRE,
        release = __ATOMIC_RELEASE,
        acq_rel = __ATOMIC_ACQ_REL,
        seq_cst = __ATOMIC_SEQ_CST,
    };

    inline constexpr memory_order memory_order_relaxed = memory_order::relaxed;
    inline constexpr memory_order memory_order_consume = memory_order::consume;
    inline constexpr memory_order memory_order_acquire = memory_order::acquire;
    inline constexpr memory_order memory_order_release = memory_order::release;
    inline constexpr memory_order memory_order_acq_rel = memory_order::acq_rel;
    inline constexpr memory_order memory_order_seq_cst = memory_order::seq_cst;

    template <typename T>
        requires(std::is_trivially_copyable_v<T> && std::is_copy_constructible_v<T> && std::is_copy_assignable_v<T>)
    struct atomic
    {
    public:
        constexpr atomic() noexcept : _value{} {};
        constexpr atomic(T desired) noexcept : _value{desired} {}

        atomic(const atomic&) = delete;
        atomic& operator=(const atomic&) = delete;
        atomic& operator=(const atomic&) volatile = delete;

        T operator=(T desired) noexcept
        {
            store(desired);
            return desired;
        }
        T operator=(T desired) volatile noexcept
        {
            store(desired);
            return desired;
        }

        T load(mtl::memory_order order = mtl::memory_order_seq_cst) const noexcept
        {
            return __atomic_load_n(&_value, static_cast<int>(order));
        }
        T load(mtl::memory_order order = mtl::memory_order_seq_cst) const volatile noexcept
        {
            return __atomic_load_n(&_value, static_cast<int>(order));
        }

        void store(T desired, mtl::memory_order order = mtl::memory_order_seq_cst) noexcept
        {
            __atomic_store_n(&_value, desired, static_cast<int>(order));
        }
        void store(T desired, mtl::memory_order order = mtl::memory_order_seq_cst) volatile noexcept
        {
            __atomic_store_n(&_value, desired, static_cast<int>(order));
        }

        operator T() const noexcept { return load(); }
        operator T() const volatile noexcept { return load(); }

        T exchange(T desired, mtl::memory_order order = mtl::memory_order_seq_cst) noexcept
        {
            return __atomic_exchange_n(&_value, desired, static_cast<int>(order));
        }
        T exchange(T desired, mtl::memory_order order = mtl::memory_order_seq_cst) volatile noexcept
        {
            return __atomic_exchange_n(&_value, desired, static_cast<int>(order));
        }

        bool compare_exchange_strong(T& expected, T desired, mtl::memory_order success, mtl::memory_order failure) noexcept
        {
            return __atomic_compare_exchange_n(&_value, &expected, desired, false, static_cast<int>(success),
                                               static_cast<int>(failure));
        }
        bool compare_exchange_strong(T& expected, T desired, mtl::memory_order success, mtl::memory_order failure) volatile noexcept
        {
            return __atomic_compare_exchange_n(&_value, &expected, desired, false, static_cast<int>(success),
                                               static_cast<int>(failure));
        }
        bool compare_exchange_strong(T& expected, T desired, mtl::memory_order order = mtl::memory_order_seq_cst) noexcept
        {
            auto failure = order == mtl::memory_order_acq_rel   ? mtl::memory_order_acquire
                           : order == mtl::memory_order_release ? mtl::memory_order_relaxed
                                                                : order;
            return compare_exchange_strong(expected, desired, order, failure);
        }
        bool compare_exchange_strong(T& expected, T desired, mtl::memory_order order = mtl::memory_order_seq_cst) volatile noexcept
        {
            auto failure = order == mtl::memory_order_acq_rel   ? mtl::memory_order_acquire
                           : order == mtl::memory_order_release ? mtl::memory_order_relaxed
                                                                : order;
            return compare_exchange_strong(expected, desired, order, failure);
        }

        bool compare_exchange_weak(T& expected, T desired, mtl::memory_order success, mtl::memory_order failure) noexcept
        {
            return __atomic_compare_exchange_n(&_value, &expected, desired, true, static_cast<int>(success),
                                               static_cast<int>(failure));
        }
        bool compare_exchange_weak(T& expected, T desired, mtl::memory_order success, mtl::memory_order failure) volatile noexcept
        {
            return __atomic_compare_exchange_n(&_value, &expected, desired, true, static_cast<int>(success),
                                               static_cast<int>(failure));
        }
        bool compare_exchange_weak(T& expected, T desired, mtl::memory_order order = mtl::memory_order_seq_cst) noexcept
        {
            auto failure = order == mtl::memory_order_acq_rel   ? mtl::memory_order_acquire
                           : order == mtl::memory_order_release ? mtl::memory_order_relaxed
                                                                : order;
            return compare_exchange_weak(expected, desired, order, failure);
        }
        bool compare_exchange_weak(T& expected, T desired, mtl::memory_order order = mtl::memory_order_seq_cst) volatile noexcept
        {
            auto failure = order == mtl::memory_order_acq_rel   ? mtl::memory_order_acquire
                           : order == mtl::memory_order_release ? mtl::memory_order_relaxed
                                                                : order;
            return compare_exchange_weak(expected, desired, order, failure);
        }

        // TODO: should only be defined for integral types
        T fetch_add(T arg, mtl::memory_order order = mtl::memory_order_seq_cst) noexcept
        {
            return __atomic_fetch_add(&_value, arg, static_cast<int>(order));
        }
        T fetch_add(T arg, mtl::memory_order order = mtl::memory_order_seq_cst) volatile noexcept
        {
            return __atomic_fetch_add(&_value, arg, static_cast<int>(order));
        }

        T fetch_sub(T arg, mtl::memory_order order = mtl::memory_order_seq_cst) noexcept
        {
            return __atomic_fetch_sub(&_value, arg, static_cast<int>(order));
        }
        T fetch_sub(T arg, mtl::memory_order order = mtl::memory_order_seq_cst) volatile noexcept
        {
            return __atomic_fetch_sub(&_value, arg, static_cast<int>(order));
        }

        T operator++() noexcept { return __atomic_add_fetch(&_value, 1, __ATOMIC_SEQ_CST); }
        T operator++() volatile noexcept { return __atomic_add_fetch(&_value, 1, __ATOMIC_SEQ_CST); }
        T operator++(int) noexcept { return __atomic_fetch_add(&_value, 1, __ATOMIC_SEQ_CST); }
        T operator++(int) volatile noexcept { return __atomic_fetch_add(&_value, 1, __ATOMIC_SEQ_CST); }
        T operator--() noexcept { return __atomic_sub_fetch(&_value, 1, __ATOMIC_SEQ_CST); }
        T operator--() volatile noexcept { return __atomic_sub_fetch(&_value, 1, __ATOMIC_SEQ_CST); }
        T operator--(int) noexcept { return __atomic_fetch_sub(&_value, 1, __ATOMIC_SEQ_CST); }
        T operator--(int) volatile noexcept { return __atomic_sub_fetch(&_value, 1, __ATOMIC_SEQ_CST); }

        template <typename U = T>
            requires(std::is_integral_v<T>)
        T operator+=(T arg) noexcept
        {
            return fetch_add(arg) + arg;
        }

        template <typename U = T>
            requires(std::is_integral_v<T>)
        T operator+=(T arg) volatile noexcept
        {
            return fetch_add(arg) + arg;
        }

        template <typename U = T>
            requires(std::is_integral_v<T>)
        T operator-=(T arg) noexcept
        {
            return fetch_sub(arg) - arg;
        }

        template <typename U = T>
            requires(std::is_integral_v<T>)
        T operator-=(T arg) volatile noexcept
        {
            return fetch_sub(arg) - arg;
        }

    private:
        T _value;
    };

} // namespace mtl
