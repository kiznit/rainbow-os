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

#include <pthread.h>
#include <reent.h>
#include <stdlib.h>


// struct _reent -> newlib state, one per thread required
__thread struct _reent __newlib_state;


void __init_newlib()
{
    _REENT_INIT_PTR_ZEROED(&__newlib_state);
}


void __init_newlib_thread()
{
    _REENT_INIT_PTR_ZEROED(&__newlib_state);
}


struct _reent* __getreent()
{
    return &__newlib_state;
}


struct __lock
{
    pthread_mutex_t mutex;
};


struct __lock __lock___sinit_recursive_mutex  = { PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP };
struct __lock __lock___sfp_recursive_mutex    = { PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP };
struct __lock __lock___atexit_recursive_mutex = { PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP };
struct __lock __lock___at_quick_exit_mutex    = { PTHREAD_MUTEX_INITIALIZER };
struct __lock __lock___malloc_recursive_mutex = { PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP };
struct __lock __lock___env_recursive_mutex    = { PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP };
struct __lock __lock___tz_mutex               = { PTHREAD_MUTEX_INITIALIZER };
struct __lock __lock___dd_hash_mutex          = { PTHREAD_MUTEX_INITIALIZER };
struct __lock __lock___arc4random_mutex       = { PTHREAD_MUTEX_INITIALIZER };


void __retarget_lock_init(_LOCK_T* lock)
{
    *lock = calloc(1, sizeof(struct __lock));
    pthread_mutex_init(&(*lock)->mutex, NULL);
}


void __retarget_lock_close(_LOCK_T lock)
{
    pthread_mutex_destroy(&lock->mutex);
    free(lock);
}


void __retarget_lock_acquire(_LOCK_T lock)
{
    pthread_mutex_lock(&lock->mutex);
}


int __retarget_lock_try_acquire(_LOCK_T lock)
{
    return pthread_mutex_trylock(&lock->mutex) == 0;
}


void __retarget_lock_release(_LOCK_T lock)
{
    pthread_mutex_unlock(&lock->mutex);
}


void __retarget_lock_init_recursive(_LOCK_T* lock)
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

    *lock = calloc(1, sizeof(struct __lock));
    pthread_mutex_init(&(*lock)->mutex, &attr);
}


void __retarget_lock_close_recursive(_LOCK_T lock)
{
    pthread_mutex_destroy(&lock->mutex);
    free(lock);
}


void __retarget_lock_acquire_recursive(_LOCK_T lock)
{
    pthread_mutex_lock(&lock->mutex);
}


int __retarget_lock_try_acquire_recursive(_LOCK_T lock)
{
    return pthread_mutex_trylock(&lock->mutex) == 0;
}


void __retarget_lock_release_recursive(_LOCK_T lock)
{
    pthread_mutex_unlock(&lock->mutex);
}
