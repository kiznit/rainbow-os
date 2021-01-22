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
    This file provides a "fake" implementation of POSIX threads.
    This is required by some runtime components (libgcc, libstdc++).

    The future might be to add our own threading model to libgcc instead of
    relying on this fake POSIX implementation.
*/

#include <cerrno>
#include <pthread.h>
#include <kernel/task.hpp>
#include <kernel/x86/cpu.hpp>
#include <metal/arch.hpp>

extern bool g_isEarly;


// We don't support this functon in the kernel and it's fine to do nothing.
// We still need to provide it as libgcc and libstdc++ detects its presence
// to determine whether or not MT is enabled. If this function is missing,
// mutexes will not be used and chaos will ensue.
extern "C" int pthread_cancel(pthread_t thread)
{
    (void)thread;
    return 0;
}


// Once you provide pthread_cancel, libgcc will start using pthread functions, so there need
// to be some implementation... To find which functions are required, look at the disassembly
// and do a search for "call   0 ": this will show you all the locations where pthread functions
// are used and missing. To find which functions they are, one will need to guess or look at
// libgcc's source code.

extern "C" int pthread_key_create(pthread_key_t* key, void (*destructor)(void*))
{
    // TODO: implement
    (void)key;
    (void)destructor;

    return 0;
}


// extern "C" int pthread_key_delete(pthread_key_t key)
// {
//     // TODO: implement
//     (void)key;

//     return 0;
// }


// extern "C" void* pthread_getspecific(pthread_key_t key)
// {
//     // TODO: implement
//     (void)key;

//     return 0;
// }


// extern "C" int pthread_setspecific(pthread_key_t key, const void* value)
// {
//     // TODO: implement
//     (void)key;
//     (void)value;

//     return 0;
// }



// TODO: I am sad to not be re-using Spinlock() here...

extern "C" int pthread_mutex_lock(pthread_mutex_t* mutex)
{
    int result;
    while ((result = pthread_mutex_trylock(mutex)) == EBUSY)
    {
        x86_pause();
    }

    return result;
}


extern "C" int pthread_mutex_trylock(pthread_mutex_t* mutex)
{
    const auto taskId = g_isEarly ? 0 : cpu_get_data(task)->m_id;

    if (__atomic_exchange_n(mutex, taskId, __ATOMIC_ACQUIRE) == PTHREAD_MUTEX_INITIALIZER)
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
    const auto taskId = g_isEarly ? 0 : cpu_get_data(task)->m_id;

    assert((int)*mutex == taskId);

    __atomic_store_n(mutex, PTHREAD_MUTEX_INITIALIZER, __ATOMIC_RELEASE);

    return 0;
}
