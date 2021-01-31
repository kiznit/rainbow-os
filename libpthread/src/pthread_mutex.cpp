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

#include <pthread.h>
#include <cstring>
#include <cerrno>
#include <rainbow.h>

// TODO: here we probably want to use stdc atomics (atomic.h), but this is not
// available with plain newlib.

static int cmpxchg(volatile int* value, int expected, int desired)
{
    __atomic_compare_exchange_n(value, &expected, desired, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); // is this the right params?
    return expected;
}


extern "C" int pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t* attr)
{
    (void)attr; // TODO: support

    memset(mutex, 0, sizeof(*mutex));
    return 0;
}


extern "C" int pthread_mutex_destroy(pthread_mutex_t* mutex)
{
    (void)mutex;
    return 0;
}


// https://eli.thegreenplace.net/2018/basics-of-futexes/
extern "C" int pthread_mutex_lock(pthread_mutex_t* mutex)
{
    assert(mutex->type == PTHREAD_MUTEX_NORMAL);

    int c = cmpxchg(&mutex->value, 0, 1);

    // If the lock was unlocked (c == 0), there is nothing for us to do.
    if (c != 0)
    {
        do
        {
            // Wait on futex (go to state 2)
            if (c == 2 || cmpxchg(&mutex->value, 1, 2) != 0)
            {
                // TODO: handle return value?
                futex_wait(&mutex->value, 2);
            }

            // Either:
            // 1) the mutex was in fact unlocked
            // 2) we slept and got unblocked
        } while ((c = cmpxchg(&mutex->value, 0, 2)) != 0);
    }

    return 0;
}


extern "C" int pthread_mutex_trylock(pthread_mutex_t* mutex)
{
    assert(mutex->type == PTHREAD_MUTEX_NORMAL);

    return cmpxchg(&mutex->value, 0, 1) == 0;
}


extern "C" int pthread_mutex_unlock(pthread_mutex_t* mutex)
{
    assert(mutex->type == PTHREAD_MUTEX_NORMAL);

    if (__atomic_fetch_sub(&mutex->value, 1, __ATOMIC_SEQ_CST) != 1)
    {
        __atomic_store_n(&mutex->value, 0, __ATOMIC_RELEASE);

        // TODO: handle return value?
        futex_wake(&mutex->value);
    }

    return 0;
}
