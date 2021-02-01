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

#include <limits.h>
#include <reent.h>
#include <stdlib.h>
#include <sys/lock.h>
#include <rainbow.h>



// struct _reent -> newlib state, one per thread required
__thread struct _reent _newlib_state;


void _init_newlib()
{
    _REENT_INIT_PTR_ZEROED(&_newlib_state);
}


void _init_newlib_thread()
{
    _REENT_INIT_PTR_ZEROED(&_newlib_state);
}


struct _reent* __getreent()
{
    return &_newlib_state;
}



struct __lock
{
    volatile int value;     // Lock is held?
    int owner;              // Thread id (recursive locks only)
    int count;              // Lock count (recursive locks only)
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


void __retarget_lock_init(_LOCK_T* lock)
{
    *lock = calloc(1, sizeof(struct __lock));
}


void __retarget_lock_close(_LOCK_T lock)
{
    free(lock);
}


void __retarget_lock_acquire(_LOCK_T lock)
{
    while (!__retarget_lock_try_acquire(lock))
    {
        // TODO: we need a proper OS mutex here
        syscall0(SYSCALL_YIELD);
    }
}


int __retarget_lock_try_acquire(_LOCK_T lock)
{
    if (!__sync_lock_test_and_set(&lock->value, 1))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}


void __retarget_lock_release(_LOCK_T lock)
{
    __sync_lock_release(&lock->value);
}


void __retarget_lock_init_recursive(_LOCK_T* lock)
{
    *lock = calloc(1, sizeof(struct __lock));
}


void __retarget_lock_close_recursive(_LOCK_T lock)
{
    free(lock);
}


void __retarget_lock_acquire_recursive(_LOCK_T lock)
{
    while (!__retarget_lock_try_acquire_recursive(lock))
    {
        // TODO: we need a proper OS mutex here
        syscall0(SYSCALL_YIELD);
    }
}


int __retarget_lock_try_acquire_recursive(_LOCK_T lock)
{
    __retarget_lock_acquire(lock);

    if (lock->owner == 0)
    {
        lock->owner = GetUserTask()->id;
        lock->count = 1;
        __retarget_lock_release(lock);
        return 1;
    }
    else if (lock->owner == GetUserTask()->id && lock->count < INT_MAX)
    {
        ++lock->count;
        __retarget_lock_release(lock);
        return 1;
    }
    else
    {
        __retarget_lock_release(lock);
        return 0;
    }
}


void __retarget_lock_release_recursive(_LOCK_T lock)
{
    __retarget_lock_acquire(lock);

    if (--lock->count == 0)
    {
        lock->owner = 0;
    }

    __retarget_lock_release(lock);
}
