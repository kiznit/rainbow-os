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
    https://pubs.opengroup.org/onlinepubs/9699919799/
    https://pubs.opengroup.org/onlinepubs/7908799/xsh/pthread.h.html
*/

#ifndef __PTHREAD_H
#define __PTHREAD_H

#include <sched.h>
#include <time.h>
// TODO: newlib wants to #include <sys/_pthreadtypes.h>, but I am not a fan
#include <sys/_pthreadtypes.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
    Conditions
*/

#define PTHREAD_COND_INITIALIZER  { 0, 0 }

int pthread_cond_init(pthread_cond_t* cond, const pthread_condattr_t* attr);
int pthread_cond_destroy(pthread_cond_t* cond);
int pthread_cond_broadcast(pthread_cond_t* cond);
int pthread_cond_signal(pthread_cond_t* cond);
int pthread_cond_timedwait(pthread_cond_t* cond, pthread_mutex_t* mutex, const struct timespec* abstime);
int pthread_cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex);


/*
    Keys (i.e. TLS)
*/

int pthread_key_create(pthread_key_t* key, void (*destructor)(void*));
int pthread_key_delete(pthread_key_t key);
void* pthread_getspecific(pthread_key_t key);
int pthread_setspecific(pthread_key_t key, const void* value);


/*
    Mutex
*/

enum
{
    PTHREAD_MUTEX_NORMAL,
    PTHREAD_MUTEX_RECURSIVE,
    PTHREAD_MUTEX_ERRORCHECK,
    PTHREAD_MUTEX_DEFAULT = PTHREAD_MUTEX_NORMAL
};

#define PTHREAD_MUTEX_INITIALIZER { PTHREAD_MUTEX_NORMAL, 0, 0, 0 }
#define PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP { PTHREAD_MUTEX_RECURSIVE, 0, 0, 0 }
#define PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP { PTHREAD_MUTEX_ERRORCHECK, 0, 0, 0 }

int pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t* attr);
int pthread_mutex_destroy(pthread_mutex_t* mutex);
int pthread_mutex_lock(pthread_mutex_t* mutex);
int pthread_mutex_trylock(pthread_mutex_t* mutex);
int pthread_mutex_unlock(pthread_mutex_t* mutex);

int pthread_mutexattr_init(pthread_mutexattr_t* attr);
int pthread_mutexattr_destroy(pthread_mutexattr_t* attr);
int pthread_mutexattr_gettype(const pthread_mutexattr_t* attr, int* type);
int pthread_mutexattr_settype(pthread_mutexattr_t* attr, int type);


/*
    Once
*/

#define PTHREAD_ONCE_INIT { 0 }

int pthread_once(pthread_once_t* once_control, void (*init_routine)(void));


/*
    R/W Lock
*/

#define PTHREAD_RWLOCK_INITIALIZER { 0, 0, 0, 0 }

int pthread_rwlock_init(pthread_rwlock_t* rwlock, const pthread_rwlockattr_t* attr);
int pthread_rwlock_destroy(pthread_rwlock_t* rwlock);
int pthread_rwlock_rdlock(pthread_rwlock_t* rwlock);
int pthread_rwlock_tryrdlock(pthread_rwlock_t* rwlock);
int pthread_rwlock_wrlock(pthread_rwlock_t* rwlock);
int pthread_rwlock_trywrlock(pthread_rwlock_t* rwlock);
int pthread_rwlock_unlock(pthread_rwlock_t* rwlock);


/*
    Threads
*/

int pthread_create(pthread_t* thread, const pthread_attr_t* attr, void* (*start_routine)(void*), void* arg);
int pthread_cancel(pthread_t thread);
int pthread_detach(pthread_t thread);
void pthread_exit(void* retval);
int pthread_join(pthread_t thread, void** retval);
pthread_t pthread_self(void);
int pthread_equal(pthread_t t1, pthread_t t2);


#ifdef __cplusplus
}
#endif

#endif
