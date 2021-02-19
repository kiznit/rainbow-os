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
#include <errno.h>
#include <string.h>

/*
    TODO: implement, for now we just make rwlock a normal mutex which is not efficient at all
*/


int pthread_rwlock_init(pthread_rwlock_t* rwlock, const pthread_rwlockattr_t* attr)
{
    (void)attr; // TODO: support

    memset(rwlock, 0, sizeof(*rwlock));
    return 0;
}


int pthread_rwlock_destroy(pthread_rwlock_t* rwlock)
{
    (void)rwlock;
    return 0;
}


int pthread_rwlock_rdlock(pthread_rwlock_t* rwlock)
{
    pthread_mutex_t* mutex = (pthread_mutex_t*)rwlock;
    return pthread_mutex_lock(mutex);
}


int pthread_rwlock_tryrdlock(pthread_rwlock_t* rwlock)
{
    pthread_mutex_t* mutex = (pthread_mutex_t*)rwlock;
    return pthread_mutex_trylock(mutex);
}


int pthread_rwlock_wrlock(pthread_rwlock_t* rwlock)
{
    pthread_mutex_t* mutex = (pthread_mutex_t*)rwlock;
    return pthread_mutex_lock(mutex);
}


int pthread_rwlock_trywrlock(pthread_rwlock_t* rwlock)
{
    pthread_mutex_t* mutex = (pthread_mutex_t*)rwlock;
    return pthread_mutex_trylock(mutex);
}


int pthread_rwlock_unlock(pthread_rwlock_t* rwlock)
{
    pthread_mutex_t* mutex = (pthread_mutex_t*)rwlock;
    return pthread_mutex_unlock(mutex);
}
