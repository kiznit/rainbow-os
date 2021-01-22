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
#include <cerrno>
#include <rainbow.h>


extern "C" int pthread_mutex_lock(pthread_mutex_t* mutex)
{
    int result;

    while ((result = pthread_mutex_trylock(mutex)) == EBUSY)
    {
        // TODO: need kernel support to properly block
        syscall0(SYSCALL_YIELD);
    }

    return result;
}


extern "C" int pthread_mutex_trylock(pthread_mutex_t* mutex)
{
    // TODO: need a gettid() function...
    const auto threadId = GetUserTask()->id;

    if (__atomic_exchange_n(mutex, threadId, __ATOMIC_ACQUIRE) == PTHREAD_MUTEX_INITIALIZER)
    {
        return 0;
    }
    else
    {
        return EBUSY;
    }
}


extern "C" int pthread_mutex_unlock(pthread_mutex_t* mutex)
{
    __atomic_store_n(mutex, PTHREAD_MUTEX_INITIALIZER, __ATOMIC_RELEASE);

    return 0;
}
