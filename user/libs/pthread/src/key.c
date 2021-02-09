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

extern pthread_mutex_t __thread_list_lock;


typedef void (*destructor_t)(void*);

static destructor_t __keys[PTHREAD_KEYS_MAX];
static pthread_rwlock_t __keyLock = PTHREAD_RWLOCK_INITIALIZER;
static int __nextKey = 0;


static void dummy_destructor(void* dummy)
{
    (void)dummy;
}


int pthread_key_create(pthread_key_t* key, void (*destructor)(void*))
{
    if (!destructor) destructor = dummy_destructor;

    pthread_rwlock_wrlock(&__keyLock);

    pthread_key_t i = __nextKey;

    do
    {
        if (!__keys[i])
        {
            __keys[i] = destructor;
            __nextKey = i;

            pthread_rwlock_unlock(&__keyLock);

            *key = i;
            return 0;
        }

        i = (i + 1) % PTHREAD_KEYS_MAX;
    } while (i != __nextKey);

    pthread_rwlock_unlock(&__keyLock);

    return EAGAIN;
}


int pthread_key_delete(pthread_key_t key)
{
    pthread_t self = pthread_self();
    pthread_t thread = self;

    pthread_rwlock_wrlock(&__keyLock);

    pthread_mutex_lock(&__thread_list_lock);
    do
    {
        thread->keyValues[key] = 0;
        thread = thread->next;
    } while (thread != self);
    pthread_mutex_unlock(&__thread_list_lock);

    __keys[key] = 0;
    pthread_rwlock_unlock(&__keyLock);

    return 0;
}


void* pthread_getspecific(pthread_key_t key)
{
    pthread_t self = pthread_self();
    return self->keyValues[key];
}


int pthread_setspecific(pthread_key_t key, const void* value)
{
    pthread_t self = pthread_self();
    self->keyValues[key] = (void*)value;

    return 0;
}


void __pthread_run_destructors()
{
    pthread_t self = pthread_self();

    for (int j = 0; j != PTHREAD_DESTRUCTOR_ITERATIONS; ++j)
    {
        pthread_rwlock_rdlock(&__keyLock);
        for (int i = 0; i != PTHREAD_KEYS_MAX; ++i)
        {
            void* value = self->keyValues[i];
            destructor_t destructor = __keys[i];

            self->keyValues[i] = 0;

            if (value && destructor && destructor != dummy_destructor)
            {
                pthread_rwlock_unlock(&__keyLock);
                destructor(value);
                pthread_rwlock_rdlock(&__keyLock);
            }
        }
        pthread_rwlock_unlock(&__keyLock);
    }
}
