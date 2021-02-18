// /*
//     Copyright (c) 2021, Thierry Tremblay
//     All rights reserved.

//     Redistribution and use in source and binary forms, with or without
//     modification, are permitted provided that the following conditions are met:

//     * Redistributions of source code must retain the above copyright notice, this
//       list of conditions and the following disclaimer.

//     * Redistributions in binary form must reproduce the above copyright notice,
//       this list of conditions and the following disclaimer in the documentation
//       and/or other materials provided with the distribution.

//     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//     AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//     IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//     DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
//     FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
//     DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//     SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
//     CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
//     OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
//     OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// */

#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <rainbow/rainbow.h>


// TODO: refactor code to eliminate this "helper"?
static inline int cmpxchg(atomic_int* value, int expected, int desired)
{
    atomic_compare_exchange_strong_explicit(value, &expected, desired, memory_order_acq_rel, memory_order_acquire);
    return expected;
}


int pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t* attr)
{
    memset(mutex, 0, sizeof(*mutex));

    if (attr)
    {
        mutex->type = *attr;
    }

    return 0;
}


int pthread_mutex_destroy(pthread_mutex_t* mutex)
{
    (void)mutex;
    return 0;
}


int pthread_mutex_lock(pthread_mutex_t* mutex)
{
    const pthread_t self = pthread_self();

    if (mutex->owner == self->id)
    {
        if (mutex->type == PTHREAD_MUTEX_RECURSIVE)
        {
            if (mutex->count < INT_MAX)
            {
                ++mutex->count;
                return 0;
            }

            return EAGAIN;
        }

        return EDEADLK;
    }

    // State 0: unlocked
    // State 1: locked, no contention
    // State 2: locked, contention
    int value = cmpxchg(&mutex->value, 0, 1);

    while (value != 0)
    {
        // We didn't get the lock, go to state 2 (contention)
        if (value == 2 || cmpxchg(&mutex->value, 1, 2) != 0)
        {
            __futex_wait((int*)&mutex->value, 2);
        }

        // Try to get the lock again
        value = cmpxchg(&mutex->value, 0, 2);
    }

    mutex->owner = self->id;

    return 0;
}


int pthread_mutex_trylock(pthread_mutex_t* mutex)
{
    pthread_t self = pthread_self();

    if (mutex->type == PTHREAD_MUTEX_RECURSIVE && mutex->owner == self->id)
    {
        if (mutex->count < INT_MAX)
        {
            ++mutex->count;
            return 0;
        }

        return EAGAIN;
    }

    if (cmpxchg(&mutex->value, 0, 1) == 0)
    {
        mutex->owner = self->id;
        return 0;
    }

    return EBUSY;
}


int pthread_mutex_unlock(pthread_mutex_t* mutex)
{
    pthread_t self = pthread_self();

    if (mutex->owner != self->id)
    {
        return EPERM;
    }

    if (mutex->type == PTHREAD_MUTEX_RECURSIVE && mutex->count > 0)
    {
        --mutex->count;
        return 0;
    }

    mutex->owner = 0;

    if (atomic_fetch_sub_explicit(&mutex->value, 1, memory_order_acq_rel) != 1)
    {
        // Previous state was 2 (locked, contention), set state to 0 (unlocked)
        atomic_store_explicit(&mutex->value, 0, memory_order_release);

        // There was contention, wakeup one thread
        __futex_wake((int*)&mutex->value, 1);
    }

    return 0;
}



int pthread_mutexattr_init(pthread_mutexattr_t* attr)
{
    if (!attr)
    {
        return EINVAL;
    }

    *attr = PTHREAD_MUTEX_DEFAULT;
    return 0;
}


int pthread_mutexattr_destroy(pthread_mutexattr_t* attr)
{
    if (!attr)
    {
        return EINVAL;
    }

    *attr = PTHREAD_MUTEX_DEFAULT;
    return 0;
}


int pthread_mutexattr_gettype(const pthread_mutexattr_t* attr, int* type)
{
    if (!attr || !type)
    {
        return EINVAL;
    }

    *type = *attr;
    return 0;
}


int pthread_mutexattr_settype(pthread_mutexattr_t* attr, int type)
{
    if (!attr || type < 0 || type > PTHREAD_MUTEX_ERRORCHECK)
    {
        return EINVAL;
    }

    *attr = type;
    return 0;
}

