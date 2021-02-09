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


/*
    TODO: implementation incomplete, untested and bugged
*/


int pthread_cond_init(pthread_cond_t* cond, const pthread_condattr_t* attr)
{
    (void)cond;
    (void)attr;
    assert(0);
    return ENOSYS;
}


int pthread_cond_destroy(pthread_cond_t* cond)
{
    (void)cond;
    assert(0);
    return ENOSYS;
}


int pthread_cond_broadcast(pthread_cond_t* cond)
{
    (void)cond;
    assert(0);
    return ENOSYS;
}


int pthread_cond_signal(pthread_cond_t* cond)
{
    (void)cond;
    assert(0);
    return ENOSYS;
}


int pthread_cond_timedwait(pthread_cond_t* cond, pthread_mutex_t* mutex, const struct timespec* abstime)
{
    (void)cond;
    (void)mutex;
    (void)abstime;
    assert(0);
    return ENOSYS;
}


int pthread_cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex)
{
    (void)cond;
    (void)mutex;
    assert(0);
    return ENOSYS;
}
