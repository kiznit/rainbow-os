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

/*
    This file provides locking functionality for Newlib.

    Right now it is all commented out because we don't need it.

    We don't need it because we use a "big kernel lock" and it is not possible for
    multiple tasks to be running inside the kernel (and using newlib) at the same time.

    The day we want some concurrency in the kernel, we need to either ditch newlib
    and replace it with something more appropriate (likely our own implementation) or
    figure a few things out.

    Using RecursiveSpinlock<TaskOwnership> is what we want, but it is causing problems
    at initialization time. Specifically, Newlib will attempt to acquire some of the
    recursive locks *before* we get to initialize the per-cpu data and Task 0. This
    means that RecursiveSpinlock<TaskOwnership>::GetOwner() basically reads garbage at
    some random memory address. It wasn't crashing when I wrote this code, but it was
    reading whatever was at [gs:0x14] (0x14 being the offset of "task" in Cpu).

    Solving this would mean to provide fake Cpu and Task objects very early in the startup
    sequence so that the locks can be acquired. This doesn't sound appealing. Alternatively
    we could have a flag to indicate whether or not Cpu and Task 0 are intitialized. if
    the flag is not set, we use a default id of "0" for ownership. If the flag is set, we
    can read the task's id normally with cpu_get_data(task)->m_id.

    But again, we don't need this just yet, so I'll just ignore it for now.
*/

/*
#include <memory>
#include <new>
#include <sys/lock.h>
#include <kernel/spinlock.hpp>



struct __lock
{
    __lock() {};

    union
    {
        Spinlock            simple;
        RecursiveSpinlock<> recursive;
    };
};


struct __lock __lock___sinit_recursive_mutex;
struct __lock __lock___sfp_recursive_mutex;
struct __lock __lock___atexit_recursive_mutex;
struct __lock __lock___at_quick_exit_mutex;
struct __lock __lock___malloc_recursive_mutex;
struct __lock __lock___env_recursive_mutex;
struct __lock __lock___tz_mutex;
struct __lock __lock___dd_hash_mutex;
struct __lock __lock___arc4random_mutex;


__attribute__((constructor))
static void init_retarget_locks()
{
    new (&__lock___sinit_recursive_mutex .recursive) RecursiveSpinlock<>();
    new (&__lock___sfp_recursive_mutex   .recursive) RecursiveSpinlock<>();
    new (&__lock___atexit_recursive_mutex.recursive) RecursiveSpinlock<>();
    new (&__lock___at_quick_exit_mutex   .simple   ) Spinlock();
    new (&__lock___malloc_recursive_mutex.recursive) RecursiveSpinlock<>();
    new (&__lock___env_recursive_mutex   .recursive) RecursiveSpinlock<>();
    new (&__lock___tz_mutex              .simple   ) Spinlock();
    new (&__lock___dd_hash_mutex         .simple   ) Spinlock();
    new (&__lock___arc4random_mutex      .simple   ) Spinlock();
}


extern "C" void __retarget_lock_init(_LOCK_T* lock)
{
    std::unique_ptr<__lock> p(new __lock());
    new (&p->simple) Spinlock();
    *lock = p.release();
}


extern "C" void __retarget_lock_close(_LOCK_T lock)
{
    lock->simple.~Spinlock();
    delete lock;
}


extern "C" void __retarget_lock_acquire(_LOCK_T lock)
{
    lock->simple.lock();
}


extern "C" int __retarget_lock_try_acquire(_LOCK_T lock)
{
    return lock->simple.try_lock();
}


extern "C" void __retarget_lock_release(_LOCK_T lock)
{
    lock->simple.unlock();
}


extern "C" void __retarget_lock_init_recursive(_LOCK_T* lock)
{
    std::unique_ptr<__lock> p(new __lock());
    new (&p->recursive) RecursiveSpinlock();
    *lock = p.release();
}


extern "C" void __retarget_lock_close_recursive(_LOCK_T lock)
{
    lock->recursive.~RecursiveSpinlock();
    delete lock;
}


extern "C" void __retarget_lock_acquire_recursive(_LOCK_T lock)
{
    lock->recursive.lock();
}


extern "C" int __retarget_lock_try_acquire_recursive(_LOCK_T lock)
{
    return lock->recursive.try_lock();
}


extern "C" void __retarget_lock_release_recursive(_LOCK_T lock)
{
    lock->recursive.unlock();
}
*/
