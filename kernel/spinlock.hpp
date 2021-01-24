/*
    Copyright (c) 2020, Thierry Tremblay
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

#ifndef _RAINBOW_KERNEL_SPINLOCK_HPP
#define _RAINBOW_KERNEL_SPINLOCK_HPP

#include <atomic>
#include <kernel/task.hpp>
#include <kernel/x86/cpu.hpp>

extern bool g_isEarly;


// Spinlocks implement busy-waiting. This means the current CPU will loop until
// it can obtain the lock and will not block / yield to another task.
//
// To prevent deadlocks, it is important that a task holding a spinlock does not
// get preempted. For this reason, interrupts need to be disabled before attempting
// the lock.
//
// To prevent deadlocks, a task holding the spinlock must not yield to another task.
//
// Spinlocks are not "fair". Multiple CPUs trying to lock the same spinlock
// could cause starvation for the current task / CPU.

class Spinlock
{
public:
    void lock();
    bool try_lock();
    void unlock();

private:
    std::atomic_bool m_lock;
};


// A recursive Spinlock
// TODO: surely this can be optimized

class RecursiveSpinlockImpl
{
public:
    RecursiveSpinlockImpl();

    void lock(int owner);
    bool try_lock(int owner);
    void unlock(int owner);

    // This is not reliable and should not be used for logic.
    // But it is useful for assertions.
    int owner() const       { return m_owner; }

private:
    Spinlock    m_lock;
    int         m_owner;    // This is the CPU id
    int         m_count;
};


struct CpuOwnership
{
    int GetOwner() const { return g_isEarly ? 0 : cpu_get_data(id); }
};


struct TaskOwnership
{
    int GetOwner() const { return g_isEarly ? 0 : cpu_get_data(task)->m_id; }
};


template<typename OwnershipPolicy = TaskOwnership>
class RecursiveSpinlock : private RecursiveSpinlockImpl, private OwnershipPolicy
{
public:
    void lock()             { RecursiveSpinlockImpl::lock(GetOwner()); }
    bool try_lock()         { return RecursiveSpinlockImpl::try_lock(GetOwner()); }
    void unlock()           { RecursiveSpinlockImpl::unlock(GetOwner()); }

    // This is not reliable and should not be used for logic.
    // But it is useful for assertions.
    int owner() const       { return RecursiveSpinlockImpl::owner(); }

private:
    using OwnershipPolicy::GetOwner;
};


#endif
